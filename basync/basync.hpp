/*
(Zlib like licence)

Copyright (c) <2018> <Jonas Lund <whizzter@gmail.com>>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

*/

#pragma once

#include <memory>
#include <functional>

// The BASYNC macro is used to define an async method
// (returntype, name/arg , variables, body)
// the variables block is needed as otherwise variables in the body would be killed
//
// (technically it uses a combination of Simon Tathams coroutine hack together with
//  C++ lambdas to create a smooth method definition that keeps state.)
#define BASYNC(rt,nmargs,vars,body) basync::promise<rt> nmargs { int __BASYNC_LABEL=-1; vars  return basync::wrap<rt>([=](std::function<void()> & __BASYNC_ACTIVATOR) mutable ->basync::result<rt> { switch (__BASYNC_LABEL) { default:  body } } ); }

// should invoke aspe that produces a promise, IFF the promise is done then we get a result immediately, otherwise execution is postponed for later time via a lambda that should re-activate the method
#define BAWAIT(ov,aspe) do { { \
 auto __BASYNC_PROM=aspe;\
 if (__BASYNC_PROM.done()) {\
   __BASYNC_PROM.then([&](auto&&v){ov=v;});\
 } else {\
   __BASYNC_LABEL=__LINE__;\
   __BASYNC_PROM.then([&](auto&&v){ov=v; __BASYNC_ACTIVATOR(); });\
   return basync::notdone{};\
 }\
 } case __LINE__:;  } while(0)

// we have a promise implementation here as well as the implementation of wrap as used by the BASYNC macro
namespace basync {
	// promises are result handlers that makes sure that results can be deliviered at later points, this variant also has direct-result optimizations compatible with the BAWAIT macro
	template<class T>
	class promise {
		struct state {
			std::shared_ptr<void> ctx=nullptr;
			std::function<void(T)> onthen=nullptr;
			std::unique_ptr<T> value=nullptr;
			
			void resolve(const T& v) {
				if (onthen)
					onthen(v);
				else
					value=std::unique_ptr<T>(new T(v));
				onthen=nullptr; // do not resolve twice to an object that was waiting.
			}
		};
		std::shared_ptr<state> st;
	public:
		promise() : st(new state) {}
		promise(const T& v) : st(new state) {
			resolve(v);
		}

		inline bool done() {
			return st->value!=nullptr;
		}

		promise& setcontext(std::shared_ptr<void> ctx) {
			st->ctx=ctx;
			return *this;
		}
		inline promise& then(std::function<void(T)> f) {
			if (st->value)
				f(*(st->value));
			else
				st->onthen=f;
			return *this;
		}
		promise& resolve(const T& v) {
			st->resolve(v);
			return *this;
		}

		// a weak-resolver is a function that does not own the promise but can resolve it's value as long as
		// any promise is still alive awaiting it's resolution.
		std::function<void(const T& v)> weakresolver() {
			std::weak_ptr<state> outv(st);
			return [outv](const T& v){
				if (auto p=outv.lock()) {
					p->resolve(v);
				}
			};
		}
	};



	// dummy class used as a pause indicator by the BAWAIT macro
	class notdone {};

	// results are produced internally by the wrapped lambdas, if produced as notdone then we know we need to await futher data before continuing otherwise proceed.
	template<class T>
	class result {
		bool valid;
		T value;
	public:
		result(notdone dummy) { valid=false; }
		result(const T&& v) { valid=true; value=v; }
		result(T&& v) { valid=true; value=std::move(v); }

		operator bool() {
			return valid;
		}
		T& get() {
			return value;
		}
	};

	template<class T,class Y>
	promise<T> wrap(Y&& v) {
		// we have the execution state of each wrapped async invocation
		struct state {
			Y v;
			std::function<void(const T&)> resf;
			std::function<void()> activate;
			state(Y&& iv) : v(std::forward<Y>(iv)) {
			}
		};
		// create a stored state with the lambda
		auto sval=std::make_shared<state>(std::forward<Y>(v));
		// create an output promise
		promise<T> out{};
		// keep the state alive within our promise(s)
		out.setcontext(sval);
		// register a weak resolver so that we can resolve the values of the promises w/o keeping the promises or ourself alive via a circular reference.
		sval->resf=out.weakresolver();
		// produce a weak reference to the state so that the activation function won't keep it alive via a circular ref.
		std::weak_ptr<state> weaksval(sval);
		// finally produce an activation function that is called for re-activations.
		sval->activate=[weaksval](){
			if (auto sval=weaksval.lock()) {
				// invoke the actual function
				if (auto res=sval->v(sval->activate)) {
					// if the actual function finished execution produce a final value
					sval->resf(res.get());
				}
			}
		};
		// run the activtion function one time to start the async method.
		sval->activate();
		// and return the created promise (that MIGHT or might NOT be initialized with a finished value)
		return out;
	}
	//class 
}
