#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

int main()
{

  vector<string> built_in;

  built_in.push_back("echo");
  built_in.push_back("exit");
  built_in.push_back("type");

  cout << unitbuf;
  cerr << unitbuf;
  while (true)
  {
    cout << "$ ";
    string input;
    getline(cin, input);
    if (input == "exit")
    {
      break;
    }
    else if (input.substr(0, 5) == "echo ")
    {
      cout << input.substr(5) << endl;
    }
    else if (input.substr(0, 5) == "type ")
    {
      string target = input.substr(5);
      if (find(built_in.begin(), built_in.end(), target) != built_in.end())
      {
        cout << target << " is a shell builtin" << endl;
      }
      else
      {
        cout << target << ": not found" << endl;
      }
    }else{
  cout << input << ": command not found" <<endl;
    }

  
  }
}
