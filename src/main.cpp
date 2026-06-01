#include <iostream>  // reading and printing 
#include <string> 
#include <vector>
#include <algorithm> // for find 
#include<sstream> // split string 
#include<cstdlib>   // reads environment variables from the operating system
#include<unistd.h> // Checks if executable file exists
#include <sys/wait.h> // make parent wait for child process
#include<fcntl.h>
#include<sys/stat.h>
using namespace std;

vector<string> parseInput(string input){
  vector<string> result;
  string current = "";
  bool inSingleQuote = false;
  bool inDoubleQuote = false;
  bool escaped = false;
  for(int i= 0 ;i<input.size();i++)
  {
    char ch = input[i];
    if(escaped){
      current += ch;
      escaped = false;
      continue;
    }
    if(inDoubleQuote){
      if(ch == '\\' && i+1<input.size()){
        char next = input[i+1];
        if(next ==  '"' || next == '\\'){
          current += next;
          i++;
          continue;
        }
        current += '\\';
        current += next;
        i++;
        continue;
      }
      if(ch == '"'){
        inDoubleQuote = false;
        continue;
      }
      current += ch;
      continue;
    }
    if(ch == '\\' && !inSingleQuote){
      escaped = true;
      continue;
    }
    if(ch == '\'' && !inDoubleQuote){
      inSingleQuote = !inSingleQuote;
      continue;
    }
    if(ch == '"' && !inSingleQuote){
    inDoubleQuote = true;
    continue;
    }
    if((ch == ' ' || ch == '\t')  && !inDoubleQuote&& !inSingleQuote){
      if(!current.empty()){
        result.push_back(current);  
        current.clear();
      }
    }
    else {
      current += ch;
    }
  }
  if(!current.empty()){
    result.push_back(current);
  }
  return result;
}


int main()
{
  vector<string> built_in; // create empty vector

  built_in.push_back("echo"); // add  values
  built_in.push_back("exit");
  built_in.push_back("type");
  built_in.push_back("pwd");
  built_in.push_back("cd");

  cout << unitbuf;
  cerr << unitbuf;
  while (true)
  {
    cout << "$ ";
    string input;
    getline(cin, input);
    if(input.empty()){ //agr enter type krde without typing  so prgm restart loop
      continue;
    }
    if (input == "exit" || input == "exit 0") //  shell stop
    {
      break;
    }
    else if (input.rfind("echo ", 0) == 0) // check first 5 letter
    {
      vector<string> commands = parseInput(input);
      string outputFile = "";
      string errorFile = "";
      bool redirect = false;
      bool errorRedirect = false;
      for(int i=0;i<commands.size();i++){
        if(commands[i] == ">" || commands[i] == "1>"){
          redirect = true;
          if(i+1< commands.size()){
            outputFile = commands[i+1];
          }
          commands.erase(commands.begin()+i,commands.begin() +i+2);
          i--;
        }
        else if(commands[i] == "2>"){
          errorRedirect = true;
          errorFile = commands[i+1];
          commands.erase(commands.begin()+i,commands.begin()+i+2);
          i--;
        }
      }
      int oldStdout = -1;
      if(redirect){
        oldStdout = dup(STDOUT_FILENO);
        int fd = open(outputFile.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);
        if(fd == -1){
          continue;
        }
        dup2(fd,STDOUT_FILENO);
        close(fd);
      } 
      for(int i=1;i<commands.size();i++){
        if(i>1){
          cout<< " ";
        }
        cout<< commands[i];
      }
      cout<<endl;
      if(redirect){
        dup2(oldStdout,STDOUT_FILENO);
        close(oldStdout);
      }
      if(errorRedirect){
        int fd = open(errorFile.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);
        if(fd != -1){
          close(fd);
        }
      }
    }
    else if(input == "pwd"){
      char cwd[1024];
      getcwd(cwd,sizeof(cwd));
      cout<<cwd<<endl;
    }
    else if(input.rfind("cd " , 0) == 0){
      string directory = input.substr(3);
      if(directory == "~"){
        char* home = getenv("HOME");
        if(home != nullptr){
          directory = home;
        }
      }
      if(chdir(directory.c_str())!=0){
          cout << "cd: " << directory
               << ": No such file or directory" << endl;

      }
    }
     else if (input.rfind("type ", 0) == 0)
    {
      string target = input.substr(5);
      if (find(built_in.begin(), built_in.end(), target) != built_in.end()) //  agr result is not end
      {
        cout << target << " is a shell builtin" << endl;
      }
      else
      {
       bool found = false; // mtlb abhi tk nhi mila
       char* path_ptr = getenv("PATH"); // os give  path value
       if(path_ptr != nullptr){
          string path_env(path_ptr); // convert to string
          stringstream ss(path_env);   // split the string
          string path;
          while(getline(ss,path,':')){ //Take text from ss until : comes
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
      vector<string> commands = parseInput(input);
      string outputFile = "";
      string errorfile = "";
      bool redirect  = false;
      bool errorRedirect = false;
      for(int i=0;i<commands.size();i++){
        if(commands[i] == ">" || commands[i] == "1>"){
          redirect = true;
          if(i+1 < commands.size()){
            outputFile = commands[i+1];
          }
          commands.erase(commands.begin()+i,commands.begin() +i+2);
          i--;
        }
        else if(commands[i] == "2>"){
          errorRedirect = true;
          errorfile = commands[i+1];
          commands.erase(commands.begin()+i,commands.begin()+i+2);
          i--;
        }
      }
      if(commands.empty()){
        continue;
      }
      pid_t  pid= fork(); // creates two processes
      if(pid == 0){ // child enters here
        if(redirect){
          int fd = open(outputFile.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);
          if(fd == -1){
            exit(1);
          }
          dup2(fd,STDOUT_FILENO);
          close(fd);
        }
        if(errorRedirect){
          int fd = open(errorfile.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);
          if(fd == -1){
            exit(1);
          }
          dup2(fd,STDERR_FILENO);
          close(fd);
        }
        vector<char*> args;
        for(int i=0;i<commands.size();i++){  //Convert strings to char*
          args.push_back(&commands[i][0]); // needs NULL to know array ended.
        }
        args.push_back(nullptr);
        execvp(args[0],args.data()); // run real executable
        cerr << input << ": command not found" << endl;
        exit(1);
      }
      else{
        waitpid(pid,nullptr,0); // Parent shell waits until child finishes.
      }
    }

  }
  return 0;
}
