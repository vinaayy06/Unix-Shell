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
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <cstring>
#include<map>
using namespace std;
map<string, string> completions;
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

vector<string> getMatches(string text){
  vector<string> matches;
  if(string("echo").rfind(text,0) == 0)
    matches.push_back("echo");
  if(string("exit").rfind(text,0) == 0)
    matches.push_back("exit");
  char* pathptr  =getenv("PATH");
  if(pathptr != nullptr){
    string pathEnv(pathptr);
    stringstream ss(pathEnv);
    string dir;
    while(getline(ss,dir,':')){
      DIR* dp = opendir(dir.c_str());
      if(dp == nullptr){
        continue;
      }
      struct dirent* entry;
      while((entry = readdir(dp)) != nullptr){
        string filename = entry->d_name;
        if(filename.rfind(text,0) == 0){
          string fullPath = dir + "/" +filename;
          if(access(fullPath.c_str(),X_OK) ==0){
            matches.push_back(filename);
          }
        }
      } 
      closedir(dp);
    }
  }
  sort(matches.begin(),matches.end());
  matches.erase(unique(matches.begin(),matches.end()),matches.end());
  return matches; 
}
string longestCommonPrefix(const vector<string> &matches){
  if(matches.empty()) return "";
  string prefix = matches[0];
  for(int i=1;i<matches.size();i++){
    while (matches[i].find(prefix) !=0)
    {
      prefix.pop_back();
      if(prefix.empty())
        return "";
    }
  }
  return prefix;
}
char* filename_generator(const char* text, int state) {
  static vector<string> matches;
  static int index;
  if (state == 0) {
    matches.clear();
    index = 0;
    string input(text);
    string dirPath = ".";
    string basePath = "";
    string prefix = input;
    size_t pos  = input.find_last_of('/');
    if(pos != string:: npos){
      dirPath = input.substr(0,pos);
      if(dirPath.empty())
        dirPath = "/";
      prefix = input.substr(pos+1);
      basePath = input.substr(0,pos+1);
    }
    DIR* dp = opendir(dirPath.c_str());
    if (dp != nullptr) {
      struct dirent* entry;
      while ((entry = readdir(dp)) != nullptr) {
        string name = entry->d_name;
        if (name.rfind(prefix, 0) == 0) {
          struct stat st;
          string fullPath;
          if(dirPath == ".")
            fullPath = name;
          else
            fullPath = dirPath + "/" + name;
          if(stat(fullPath.c_str(),&st) ==0 && S_ISDIR(st.st_mode)){
            matches.push_back(basePath + name + "/"); 
          }
          else{
            matches.push_back(basePath + name);
          } 
        }
      }
      closedir(dp);
    }
    sort(matches.begin(), matches.end());
  }
  if (index < matches.size()) {
    return strdup(matches[index++].c_str());
  }
  return nullptr;
}
char* command_generator(const char* text, int state){
  static int index;
  static vector<string> matches;
  if(state == 0){
    matches = getMatches(text);
    index =0;
  }
  if(index < matches.size()){  
    return strdup(matches[index++].c_str());
  }
  return nullptr;
}
vector<string> getFileMatches(string input){
  vector<string> matches;
  string dirPath = ".";
  string basePath = "";
  string prefix = input;
  size_t pos = input.find_last_of('/');
  if(pos != string::npos){
    dirPath = input.substr(0,pos);
    if(dirPath.empty())
      dirPath = "/";
    prefix = input.substr(pos+1);
    basePath = input.substr(0,pos+1);
  }
  DIR* dp = opendir(dirPath.c_str());
  if(dp){
    struct dirent* entry;
    while((entry = readdir(dp)) != nullptr){
      string name = entry->d_name;
      if(name == "." || name == "..")
        continue;
      if(name.rfind(prefix,0) == 0){
        struct stat st;
        string fullPath;
        if(dirPath == "."){
          fullPath = name;
        }
        else{
          fullPath = dirPath + "/" + name;
        }
        if(stat(fullPath.c_str(), &st) ==0 && S_ISDIR(st.st_mode)){
          matches.push_back(basePath + name + "/");
        }
        else{
          matches.push_back(basePath + name);
        }
      }
    }
    closedir(dp);
  }
  sort(matches.begin(),matches.end());
  return matches;
}

char** my_completion(const char*text, int start, int end){
  string line  = rl_line_buffer;
  vector<string> args = parseInput(line);
   vector<string> words = parseInput(line);
   string command = "";
  string currentWord = text;
  string previousWord = "";
  if(!words.empty())
  {
    command = words[0];
  }
  if(words.size() >= 2)
  {
    previousWord = words[words.size()-2];
  }
  if(!args.empty()){
    auto it = completions.find(args[0]);
    if(it != completions.end() && start>0){
    string compLine = rl_line_buffer;
      string compPoint = to_string(rl_point);
      string cmd =  
      "COMP_LINE=\"" + compLine + "\" "
      "COMP_POINT=\"" + compPoint + "\" "
      "\"" + it->second + "\" "
      "\"" + command + "\" "
      "\"" + currentWord + "\" "
      "\"" + previousWord + "\"";
      FILE* pipe = popen(cmd.c_str(), "r");
      if(pipe){
        char buffer[1024];
        if(fgets(buffer,sizeof(buffer),pipe)){
          string completions = buffer;
          while (!completions.empty() &&
                (completions.back() == '\n' ||
                 completions.back() == '\r'))
          {
            completions.pop_back();
          }
          if (completions.rfind(currentWord, 0) == 0) 
          {
            string suffix = completions.substr(currentWord.size());
            rl_insert_text(suffix.c_str());
            rl_insert_text(" ");
            rl_redisplay();
}
        }
        pclose(pipe);
      }
      return nullptr;
    }
  }
  (void)start;
  (void)end;
  if(start > 0){
    vector<string> files;
    DIR* dp = opendir(".");
    if(dp){
      struct dirent* entry;
      while((entry = readdir(dp)) != nullptr) {
        string name = entry->d_name;
        if(name == "." || name == "..")
          continue;
        files.push_back(name);
      }
      closedir(dp);
    }
    if(files.size() == 1){
      struct stat st;
      if(stat(files[0].c_str(),&st) == 0 && S_ISDIR(st.st_mode)){
        rl_insert_text((files[0] + "/").c_str());
      }
      else{
        rl_insert_text((files[0] + " ").c_str());
      }
    }
    return nullptr;
  } 
  string prefix = text;
  vector<string> matches = getMatches(prefix);
  if(matches.empty()){
    return nullptr;
  }
  if(matches.size() ==1){
    rl_completion_append_character = ' ';
    return rl_completion_matches(text,command_generator);
  }
  string lcp = longestCommonPrefix(matches);
  if(lcp.length()> prefix.length()){
    rl_line_buffer[0] = '\0';
    rl_point = 0;
    rl_end = 0;
    rl_insert_text(lcp.c_str());
    rl_redisplay();
    return nullptr;
  }
  static string previousPrefix = "";
  static int tabPresses = 0;
  if(previousPrefix == prefix){
    tabPresses++;
  }
  else{
    previousPrefix = prefix;
    tabPresses = 1;
  }
  if(tabPresses == 1 ){
    cout << '\a';
    cout.flush();
    return nullptr;
  }
  cout << '\n';
  for(string s : matches){
    cout << s << "  ";
  }
  cout<< '\n';
  rl_on_new_line();
  rl_redisplay();
  tabPresses = 0;
  return nullptr;
}
int main()
{
  rl_attempted_completion_function = my_completion;
  rl_completion_append_character = ' ';
  vector<string> built_in; // create empty vector

  built_in.push_back("echo"); // add  values
  built_in.push_back("exit");
  built_in.push_back("type");
  built_in.push_back("pwd");
  built_in.push_back("cd");
  built_in.push_back("complete");

  cout << unitbuf;
  cerr << unitbuf;
  while (true)
  {
    char* line  = readline("$ ");
    if(line  == nullptr){
      break;
    }
    string input(line);
    if(!input.empty()){
      add_history(line);
    }
    free(line);
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
      bool appendRedirect = false;
      bool errorRedirect = false;
      bool errorAppendRedirect = false;
      for(int i=0;i<commands.size();i++){
        if(commands[i] == ">" || commands[i] == "1>"){
          redirect = true;
          outputFile = commands[i+1];
          commands.erase(commands.begin()+i,commands.begin() +i+2);
          i--;
        }
        else if(commands[i] == ">>" || commands[i] == "1>>"){
          appendRedirect = true;
          outputFile = commands[i+1];
          commands.erase(commands.begin()+i,commands.begin()+i+2);
          i--;
        }
        else if(commands[i] == "2>"){
          errorRedirect = true;
          errorFile = commands[i+1];
          commands.erase(commands.begin()+i,commands.begin()+i+2);
          i--;
        }
        else if(commands[i] == "2>>"){
          errorAppendRedirect = true;
          errorFile = commands[i+1];
          commands.erase(commands.begin()+i,commands.begin()+i+2);
          i--;
        }
      }
      int oldStdout = -1;
      if(redirect || appendRedirect){
        oldStdout = dup(STDOUT_FILENO);
        int fd;
        if(appendRedirect){
          fd = open(outputFile.c_str(),O_WRONLY | O_CREAT | O_APPEND,0644);
        }
        else{
          fd  = open(outputFile.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);
        }
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
      if(redirect || appendRedirect){
        dup2(oldStdout,STDOUT_FILENO);
        close(oldStdout);
      }
      if(errorRedirect || errorAppendRedirect){
        int fd;
        if(errorAppendRedirect){
          fd  = open(errorFile.c_str(),O_WRONLY | O_CREAT | O_APPEND,0644);
        }
        else{
          fd =  open(errorFile.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);
        }
        if(fd == -1){
         exit(1);
        }
        dup2(fd,STDERR_FILENO);
      close(fd);
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
    else if(input.rfind("complete",0) == 0){
      vector<string> args = parseInput(input);
      if(args.size() >= 4 && args[1] == "-C"){
        string scriptPath = args[2];
        string command = args[3];
        completions[command] = scriptPath;
      }
      else if(args.size()>=3 && args[1] =="-p"){
        string command = args[2];
        auto it = completions.find(command);
        if(it == completions.end()){
          cout<<"complete: "<<command
              <<": no completion specification"<<endl;
        }
        else{
          cout << "complete -C '"
               << it->second
               << "' "
               << command
               << endl;
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
      bool appendRedirect = false;
      bool errorAppendRedirect = false;
      for(int i=0;i<commands.size();i++){
        if(commands[i] == ">" || commands[i] == "1>"){
          redirect = true; 
          outputFile = commands[i+1];
          commands.erase(commands.begin()+i,commands.begin() +i+2);
          i--;
        }
        else if(commands[i] == ">>" || commands[i] == "1>>"){
          appendRedirect = true;
          outputFile = commands[i+1];
          commands.erase(commands.begin()+i, commands.begin()+i+2);
          i--;

        }
        else if(commands[i] == "2>"){
          errorRedirect = true;
          errorfile = commands[i+1];
          commands.erase(commands.begin()+i,commands.begin()+i+2);
          i--;
        }
        else if(commands[i].rfind("2>>",0) == 0){
          errorAppendRedirect = true;
          if(commands[i] == "2>>"){
            errorfile = commands[i+1];
            commands.erase(commands.begin()+i,commands.begin()+i+2);
          }
          i--;
        }
      }
      if(commands.empty()){
        continue;
      }
      pid_t  pid= fork(); // creates two processes
      if(pid == 0){ // child enters here
        if(redirect || appendRedirect){
          int fd;
          if(appendRedirect){
            fd = open(outputFile.c_str(),O_WRONLY | O_CREAT | O_APPEND,0644);
          }
          else{
            fd = open(outputFile.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);
          }
          if(fd == -1){
            exit(1);
          }
          dup2(fd,STDOUT_FILENO);
          close(fd);
        }
        if(errorRedirect || errorAppendRedirect){
          int fd;
          if(errorAppendRedirect){
            fd  = open(errorfile.c_str(), O_WRONLY | O_CREAT | O_APPEND,0644);
          }
          else{
            fd  = open(errorfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC,0644);
          }
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
