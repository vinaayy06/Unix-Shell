#include <iostream>
#include <string>
using namespace std;

int main() {
  
  cout << unitbuf;
  cerr << unitbuf;

  cout << "$ ";
  string input;
  cin >>input;
  cout<<input<<": command not found"<<endl;
}
