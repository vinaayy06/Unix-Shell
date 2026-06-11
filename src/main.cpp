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
#include<iomanip>
#include<fstream>
using namespace std;
map<string, string> completions;
map<string, string> shellVars;
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
static string lastWord = "";
static int tabCount = 0;
static int jobCounter = 0;
struct Job {
  int number;
  pid_t pid;
  string command;
  string status;
};
static vector<Job> jobs;
bool isBuiltin(const string& cmd)
{
    return cmd == "echo" ||
           cmd == "type" ||
           cmd == "pwd"  ||
           cmd == "cd"   ||
           cmd == "exit" ||
           cmd == "complete"||
           cmd == "jobs"||
           cmd == "history"||
           cmd == "declare";
}
void runBuiltin(const vector<string>& args)
{
  if(args.empty()) return;
  if(args[0] == "echo"){
    for(size_t i=1;i<args.size();i++){
      if(i>1) cout << " ";
      cout << args[i];
    }
    cout << endl;
  }
  else if(args[0] == "type"){
    string target = args[1];
    if(target=="echo" ||
       target=="exit" ||
       target=="type" ||
       target=="pwd" ||
       target=="cd" ||
       target=="complete"||
       target=="jobs" ||
       target=="history"||
       target=="declare" )
    {
      cout << target
          << " is a shell builtin"
          << endl;
    }
  }
}
vector<string> splitPipeline(const string& input){
  vector<string> result;
  string current;
  stringstream ss(input);
  while(getline(ss, current, '|')){
    while(!current.empty() && current.front() == ' ')
      current.erase(current.begin());
    while(!current.empty() && current.back() == ' ')
        current.pop_back();
    result.push_back(current);
  }
  return result;
}
char** my_completion(const char*text, int start, int end){
  rl_attempted_completion_over = 1;
  string line = rl_line_buffer;
  vector<string> words = parseInput(line);
  string command = "";
  string currentWord = text;
  string previousWord = "";
  if(!words.empty()) command = words[0];
  if(words.size() >= 2) previousWord = words[words.size()-2];
  if(!words.empty()){
    auto it = completions.end();
    for(auto &p : completions){
      if(command.rfind(p.first,0) == 0){ it = completions.find(p.first); break; }
    }
    if(it != completions.end()){
      string cmd =
        "COMP_LINE=\"" + string(rl_line_buffer) + "\" "
        "COMP_POINT=\"" + to_string(rl_point) + "\" "
        "\"" + it->second + "\" "
        "\"" + command + "\" "
        "\"" + currentWord + "\" "
        "\"" + previousWord + "\"";
      FILE* pipe = popen(cmd.c_str(), "r");
      if(pipe){
        vector<string> candidates;
        char buffer[1024];
        while(fgets(buffer, sizeof(buffer), pipe)){
          string s = buffer;
          while(!s.empty() && (s.back()=='\n' || s.back()=='\r')) s.pop_back();
          if(!s.empty()) candidates.push_back(s);
        }
        pclose(pipe);
        vector<string> filtered;
        for(auto& c : candidates)
          if(c.rfind(currentWord,0)==0) filtered.push_back(c);
        if(!filtered.empty()) candidates = filtered;
        sort(candidates.begin(), candidates.end());
        if(candidates.size() == 1){
          int sp = rl_point - currentWord.size();
          rl_delete_text(sp, rl_point); rl_point = sp;
          rl_insert_text(candidates[0].c_str());
          rl_insert_text(" ");
          rl_redisplay();
          tabCount = 0; lastWord.clear();
        } else if(candidates.size() > 1){
          string lcp = longestCommonPrefix(candidates);
          if(lcp.size() > currentWord.size()){
            int sp = rl_point - currentWord.size();
            rl_delete_text(sp, rl_point); rl_point = sp;
            rl_insert_text(lcp.c_str());
            rl_redisplay(); tabCount = 0;
          } else {
            if(lastWord == currentWord) tabCount++;
            else { lastWord = currentWord; tabCount = 1; }
            if(tabCount == 1){ cout << '\a'; cout.flush(); }
            else {
              cout << '\n';
              for(auto& c : candidates) cout << c << " ";
              cout << '\n';
              rl_on_new_line(); rl_redisplay(); tabCount = 0;
            }
          }
        }
      }
      return nullptr;
    }
  }
  if(start > 0){
    vector<string> matches = getFileMatches(currentWord);
    if(matches.empty()) return nullptr;
    if(matches.size() == 1){
      int sp = rl_point - currentWord.size();
      rl_delete_text(sp, rl_point); rl_point = sp;
      rl_insert_text(matches[0].c_str());
      if(matches[0].back() != '/') rl_insert_text(" ");
      rl_redisplay();
      return nullptr;
    }
    string lcp = longestCommonPrefix(matches);
    if(lcp.size() > currentWord.size()){
      int sp = rl_point - currentWord.size();
      rl_delete_text(sp, rl_point); rl_point = sp;
      rl_insert_text(lcp.c_str());
      rl_redisplay(); tabCount = 0; lastWord.clear();
      return nullptr;
    }
    if(lastWord == currentWord) tabCount++;
    else { lastWord = currentWord; tabCount = 1; }
    if(tabCount == 1){ cout << '\a'; cout.flush(); }
    else {
      cout << '\n';
      for(auto& m : matches) cout << m << "  ";
      cout << '\n';
      rl_on_new_line(); rl_redisplay(); tabCount = 0;
    }
    return nullptr;
  }
  vector<string> matches = getMatches(currentWord);
  if(matches.empty()) return nullptr;
  if(matches.size() == 1){
    string suffix = matches[0].substr(strlen(text));
    rl_insert_text(suffix.c_str());
    rl_insert_text(" ");
    rl_redisplay();
    return nullptr;
  }
  string lcp = longestCommonPrefix(matches);
  if(lcp.size() > currentWord.size()){
    int sp = rl_point - currentWord.size();
    rl_delete_text(sp, rl_point); rl_point = sp;
    rl_insert_text(lcp.c_str());
    rl_redisplay(); tabCount = 0; lastWord.clear();
  }
  if(lastWord == currentWord) tabCount++;
  else { lastWord = currentWord; tabCount = 1; }
  if(tabCount == 1){ cout << '\a'; cout.flush(); }
  else {
    cout << '\n';
    for(auto& m : matches) cout << m << " ";
    cout << '\n';
    rl_on_new_line(); rl_redisplay(); tabCount = 0;
  }
  return nullptr;
}
void reapJobs(){
  for(auto& j: jobs){
    if(j.status == "Running"){
      int wstatus;
      pid_t result = waitpid(j.pid, &wstatus,WNOHANG);
      if(result == j.pid && WIFEXITED(wstatus)){
        j.status = "Done";
        if(j.command.size() >=2 && j.command.substr(j.command.size() -2) == " &"){
          j.command = j.command.substr(0, j.command.size()-2);
        }
      }
    }
  }
  int total = jobs.size();
  for(int i=0;i<total ;i++){
    if(jobs[i].status == "Done"){
      char marker;
      if(i == total -1){
        marker = '+';
      }
      else if(i== total -2){
        marker = '-';
      }
      else{
        marker = ' ';
      }
      cout << "[" << jobs[i].number << "]"
           << marker << "  "
           << left << setw(24) << jobs[i].status
           << jobs[i].command << endl;
    }     
  }
  vector<Job> stillRunning;
  for(auto& j : jobs){
    if(j.status == "Running"){
      stillRunning.push_back(j);
    }
  }
  jobs = stillRunning;
}
vector<string> commandHistory;

string expandVars(const string& input){
  string result;
  int i = 0;
  while(i < (int)input.size()){
    if(input[i] == '$' && i+1 < (int)input.size()){
      if(input[i+1] == '{'){
        i += 2; 
        string name;
        while(i < (int)input.size() && input[i] != '}'){
          name += input[i++];
        }
        if(i < (int)input.size()) i++; 
        auto it = shellVars.find(name);
        if(it != shellVars.end())
        result += it->second;
      }
      else if(isalpha(input[i+1]) || input[i+1] == '_'){
        i++;
        string name;
        while(i < (int)input.size() && (isalnum(input[i]) || input[i] == '_')){
          name += input[i++];
        }
        auto it = shellVars.find(name);
        if(it != shellVars.end())
          result += it->second;
      }
      else{
        result += input[i++];
      }
    }
    else{
      result += input[i++];
    }   
  }
  return result;
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
  built_in.push_back("jobs");
  built_in.push_back("history");
  built_in.push_back("declare");

  cout << unitbuf;
  cerr << unitbuf;
  int historyLoadedCount = 0;
  char* histfile = getenv("HISTFILE");
  if(histfile != nullptr){
    read_history(histfile);
    HIST_ENTRY **hist = history_list();
    if(hist){
      while(hist[historyLoadedCount] != nullptr)
        historyLoadedCount++;
    }
  }
  while (true)
  {
    reapJobs();
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
    input = expandVars(input);  
    size_t pipePos = input.find('|');
    if(input.find('|') != string::npos){
      vector<string> pipeline = splitPipeline(input);
      int n = pipeline.size();
      vector<pid_t> pids;
      int prevRead = -1;
      for(int i =0;i<n;i++){
        vector<string> argsVec = parseInput(pipeline[i]);
        int fd[2];
        if(i != n-1){
          pipe(fd);
        }
        pid_t pid = fork();
        if(pid ==0){
          if(prevRead != -1){
            dup2(prevRead, STDIN_FILENO);
          }
          if(i != n - 1){
            dup2(fd[1], STDOUT_FILENO);
          }
          if(prevRead != -1){
            close(prevRead);
          }
          if(i != n - 1){
            close(fd[0]);
            close(fd[1]);
          }
          if(isBuiltin(argsVec[0])){
            runBuiltin(argsVec);
            exit(0);
          }
          vector<char*> args;
          for(auto &s : argsVec)
            args.push_back(&s[0]);
           args.push_back(nullptr);
          execvp(args[0], args.data());
          exit(1);
        } 
        pids.push_back(pid);
        if(prevRead != -1)
          close(prevRead);
        if(i != n - 1){
          close(fd[1]);
          prevRead = fd[0];
        } 
      }
      for(pid_t pid : pids)
        waitpid(pid, nullptr, 0);
      continue;
    }
    if (input == "exit" || input == "exit 0") //  shell stop
    {
      char* histfile = getenv("HISTFILE");
      if(histfile != nullptr){
        HIST_ENTRY **hist = history_list();
        int total = 0;
        if(hist) while(hist[total] != nullptr) total++;
        int newEntries = total - historyLoadedCount;
        if(newEntries > 0)
          append_history(newEntries, histfile);
        }
        else{
          append_history(0,histfile);
        }
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
    else if(input == "jobs"){
      for(auto& j : jobs){
        int wstatus;
        pid_t result = waitpid(j.pid, &wstatus, WNOHANG);
        if(result == j.pid && WIFEXITED(wstatus)){
          j.status = "Done";
          if(j.command.size() >= 2 && j.command.substr(j.command.size()-2) == " &"){
            j.command = j.command.substr(0, j.command.size()-2);
          }
        }
      }
      int total = jobs.size();
      for(int i=0;i<total;i++){
        char marker;
        if(i == total -1){
          marker = '+';
        }
        else if(i == total -2){
          marker = '-';
        }
        else{
          marker = ' ';
        }
        cout << "[" << jobs[i].number << "]"
             << marker << "  "
             << left << setw(24) << jobs[i].status
             << jobs[i].command << endl;
      }
      vector<Job> stillRunning;
      for(auto& j : jobs){
        if(j.status == "Running"){
          stillRunning.push_back(j);
        }
      }
      jobs = stillRunning;
    }
    else if(input.rfind("history" ,0) == 0){
      vector<string> args = parseInput(input);
      if(args.size() ==3 && args[1] == "-r"){
        string filePath = args[2];
        ifstream file(filePath);
        string line;
        while(getline(file,line)){
          if(!line.empty()){
            add_history(line.c_str());
          }
        }
      }
      else if(args.size() == 3 && args[1] == "-w"){
        string filePath = args[2];
        ofstream file(filePath);         
        HIST_ENTRY **hist = history_list();
        if(hist){
          for(int i = 0; hist[i] != nullptr; i++){
              file << hist[i]->line << "\n";
          }
        }
      }
      else if(args.size() == 3 && args[1] == "-a"){
        string filePath = args[2];
        static int lastWritten = 0;
        ofstream file(filePath, ios:: app);
        HIST_ENTRY **hist = history_list();
        if(hist){
          int total = 0;
          while(hist[total] != nullptr) total++;
          for(int i = lastWritten; i<total;i++){
            file << hist[i] ->line << "\n";
          }
          lastWritten = total;
        }
      }
      else{
        HIST_ENTRY **hist = history_list();
        if(hist){
          int total = 0;
          while(hist[total] != nullptr){
            total++;
          }
          int start = 0;
          if(args.size() == 2){
            int n = stoi(args[1]);
            if(n<total){
              start = total - n;
            }
          }
          for(int i= start ; i<total; i++){
            cout<<setw(5) << i+1 << " "<<hist[i]->line<<endl;
          }
        }
      }
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
      else if(args.size() >=3 && args[1] == "-r"){
        string command = args[2];
        completions.erase(command);
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
    else if(input.rfind("declare", 0) == 0){
      vector<string> args = parseInput(input);
      if(args.size() ==2 && args[1].find('=') != string::npos){
        size_t eq = args[1].find('=');
        string name = args[1].substr(0, eq);
        string value = args[1].substr(eq + 1);
        bool valid  = !name.empty() && (isalpha(name[0]) || name[0] == '_');
        for(int i = 1; valid && i < name.size(); i++){
          if(!isalnum(name[i]) && name[i] != '_') valid = false;
        }
        if(!valid){
          cout << "declare: `" << args[1] << "': not a valid identifier" << endl;
        }
        else{
          shellVars[name] = value;
        }
      }
      else if(args.size() == 3 && args[1] == "-p"){
        string varName = args[2];
        auto it = shellVars.find(varName);
        if(it == shellVars.end()){
          cout << "declare: " << varName << ": not found" << endl;
        }
        else{
          cout << "declare -- " << varName << "=\"" << it->second << "\"" << endl;
        }
      }
    }
    else{
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
      bool background = false;
      if(!commands.empty() && commands.back() == "&"){
        background = true;
        commands.pop_back();
      }
      if(commands.empty()) continue;
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
        if(background){
          int newNumber = 1;
          while(true){
            bool taken = false;
            for(auto& j : jobs){
              if(j.number == newNumber){
                taken = true;
                break;
              }
            } 
            if(!taken) break;
            newNumber++;
          }
          jobCounter = newNumber;
          string cmdStr = "";
          for(int i=0 ;i<commands.size();i++){
            if(i>0) cmdStr += " ";
            cmdStr += commands[i];
          }
          cmdStr += " &";
          jobs.push_back({newNumber,pid,cmdStr,"Running"});
          cout << "[" << newNumber << "] " << pid << endl;
        }
        else{
          waitpid(pid,nullptr,0);
        }
      }
    }

  }
  return 0;
}
