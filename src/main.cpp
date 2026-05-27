#include <iostream> 
#include <string>
#include <vector>
#include <algorithm> 
#include<sstream> // split string 
#include<cstdlib>   // reads environment variables from the operating system
#include<unistd.h> // Checks if executable file exists
#include <sys/wait.h> // make parent wait for child process
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
    if(input.empty()){
      continue;
    }
   if (input == "exit" || input == "exit 0")
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
       bool found = false;
       char* path_ptr = getenv("PATH");
       if(path_ptr != nullptr){
          string path_env(path_ptr);
          stringstream ss(path_env);   // split the string
          string path;
          while(getline(ss,path,':')){
            string full_path = path + "/" + target;
            if(access(full_path.c_str(),X_OK) == 0){ // check file exist & execute permission exists
              cout << target << " is " << full_path << endl;
              found = true;
              break;
            }
          }
       }
        if(!found){
          cout << target << ": not found" << endl;
        }
      }
    }
    else
    {
      stringstream ss(input);
      vector<string> commands;
      string word;
      while(ss>> word){
        commands.push_back(word);
      }
      pid_t  pid= fork();
      if(pid == 0){
        vector<char*> args;
        for(string& cmd : commands){
          args.push_back(&cmd[0]);
        }
        args.push_back(nullptr);
        execvp(args[0],args.data());
        cout << input << ": command not found" << endl;
        exit(1);
      }
      else{
        waitpid(pid,nullptr,0);
      }
    }
  }
  return 0;
}