// a compherhensive example of the async functions possible with BASYNC
#include "basync.hpp"

#include <string>
#include <iostream>
#include <vector>

// An asyncronous function needs the BASYNC macro (returntype, name/arg , variables, body)
BASYNC(
	int                 // return type
,
	test ( std::string id, int count, std::function<basync::promise<std::string>()> valuesource)
,
	int         idx;    // variable needs to be here to live across BAWAIT calls
	std::string val;    // this variable is the return value from an await!
,
	// Function body starts here!!

	std::cout<<"["<<id<<"] Entering\n";

	for (idx=0;idx<count;idx++) {
		val=std::to_string(idx);

		BAWAIT(val,valuesource()); // <-- This invocation of the BAWAIT macro will PAUSE the function until a value is provided

		int dummy=123;;
		std::cout<<"["<<id<<"] Test awaited value:"<<val<<" on iter "<<idx<<" (stackpos:"<<((void*)&dummy) << ")\n";
	}

	std::cout<<"["<<id<<"] Finished\n";
	return count*20;
)
// END OF ASYNC function

// predeclaration of 2 methods that produces promises that can be BAWAIT:ed on
basync::promise<std::string> direct();
basync::promise<std::string> pendingprod();

// pending promises to resolve (this is filled by the pending function)
std::vector<basync::promise<std::string>> pendingvalues;

int main(int argc,char **argv) {

	std::cout<<"{Main  } Entering\n";

	// storage for 2 promises
	basync::promise<int> proms[3];
	// first create one that will fetch pending promises that won't be resolved immediately
	proms[0]=test("First ",2,pendingprod);
	// the second will only request promises that will be fulfilled immediately
	proms[1]=test("Second",4,direct);
	// the third one that will also fetch pending promises that won't be resolved immediately but live longer than number 0
	proms[2]=test("Third ",6,pendingprod);

	// register handlers
	for (int i=0;i<3;i++) {
		proms[i].then([=](int v){
			std::cout<<"{Main  } Result ["<<(i+1)<<"] gotten as:"<<v<<"\n";
		});
	}

	int count=0;
	while(pendingvalues.size()) {
		// print out status of our possibly pending promises
		std::cout<<"{Main  } Pending values:"<<pendingvalues.size()<<" Done:["<<proms[0].done()<<","<<proms[1].done()<<","<<proms[2].done()<<"]\n";
		
		// produce a value from the mainloop to the async in front if needed
		pendingvalues[0].resolve("{From main loop "+std::to_string(count++)+"}");

		// erase the head as it's produced already!
		pendingvalues.erase(pendingvalues.begin());
	}

	std::cout<<"{Main  } Finished (ALL DONE!!!)";

	return 0;
}

// this function just produces values immediately
basync::promise<std::string> direct() {
	return "direct resolve!";
}

// whilst this function creates a pending value into the pendingvalues array that needs to be resolved later
basync::promise<std::string> pendingprod() {
	// make an promise that isn't finalized yet.
	pendingvalues.emplace_back();
	return pendingvalues.back();
}

