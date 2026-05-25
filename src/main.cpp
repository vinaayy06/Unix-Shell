#include <iostream>
#include <string>
using namespace std;

int main() {
  
  cout << unitbuf;
  cerr << unitbuf;
  while (true) {
    cout << "$ ";
    string input;
    getline(cin, input);
    if(input == "exit"){
      break;
    }
    else if(input.size()>=5 &&input.substr(0,5) == "echo "){
      cout << input.substr(5) << endl;
    }
    else{
      cout<<input<<endl;
    }
    
    cout<<input<<endl;
  }
}

