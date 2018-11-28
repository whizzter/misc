// a compherhensive example of the async functions possible with BASYNC
#include "basync.hpp"

#include <string>
#include <iostream>
#include <vector>

// An asyncronous function needs the BASYNC macro (returntype, name/arg , variables, body)
BASYNC(
int , test(std::string id,int a,basync::promise<std::string> (*q)()),

  int count;          // variable needs to be here to live across BAWAIT calls
  std::string val;    // this variable is the return from an await!
,
	std::cout<<"["<<id<<"] Entering\n";
	for (count=0;count<a;count++) {
		val=std::to_string(count);

		BAWAIT(val,q());

		int dummy=123;;
		std::cout<<"["<<id<<"] Test awaited value:"<<val<<" on iter "<<count<<" (stack:"<<((void*)&dummy) << ")\n";
	}
	std::cout<<"["<<id<<"] Finished\n";
	return a*20;
)
// END OF ASYNC function

// predeclaration of 2 methods that produces 
basync::promise<std::string> direct();
basync::promise<std::string> pendingprod();

// pending promises to resolve (this is filled by the pending function)
std::vector<basync::promise<std::string>> pendingvalues;

int main(int argc,char **argv) {

	// storage for 2 promises
	basync::promise<int> proms[3];
	// first create one that will fetch pending promises that won't be resolved immediately
	proms[0]=test("First",2,pendingprod);
	// the second will only request promises that will be fulfilled immediately
	proms[1]=test("Second",4,direct);
	// the third one that will also fetch pending promises that won't be resolved immediately but live longer than number 0
	proms[2]=test("Third",6,pendingprod);

	// register handlers
	for (int i=0;i<3;i++) {
		proms[i].then([=](int v){
			std::cout<<"Result ["<<(i+1)<<"] gotten as:"<<v<<"\n";
		});
	}

	int count=0;
	while(pendingvalues.size()) {
		// print out status of our possibly pending promises
		std::cout<<"Pending values:"<<pendingvalues.size()<<" Done:["<<proms[0].done()<<","<<proms[1].done()<<","<<proms[2].done()<<"]\n";
		// produce a value from the mainloop to the async
		pendingvalues[0].resolve("From main loop"+std::to_string(count++));
		// erase the head as it's produced already!
		pendingvalues.erase(pendingvalues.begin());
	}

	std::cout<<"Done!!";

	return 0;
}

basync::promise<std::string> direct() {
	return "direct resolve!";
}

basync::promise<std::string> pendingprod() {
	// make an promise that isn't finalized yet.
	pendingvalues.emplace_back();
	return pendingvalues.back();
}

