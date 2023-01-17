#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctime>
#include <fcntl.h>
#include <vector>
#include <string>

#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;

int main () {

    char prev_dir[1000];
    getcwd(prev_dir,1000);

    int std_in= dup(0);
    int std_out= dup(1);

    vector<int> dif_process;
    for (;;) {
        //absolute path to current dir
        char cwd[1000];
        getcwd(cwd, sizeof(cwd));



	    
        char s[1000];
        time_t t = time(NULL);
        struct tm * p = localtime(&t); 
        strftime(s, 1000, "%b %d %T", p);

        // need date/time, username, and absolute path to current dir
        cout << YELLOW << s << " "<< getenv("USER") << ":" << cwd << "$" << NC << " ";
        
        // get user inputted command
        string input;
        getline(cin, input);

        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

/////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (unsigned long i = 0; i < dif_process.size(); i++) { 
            if(waitpid(dif_process[i], 0, WNOHANG) == dif_process[i]) {
                dif_process.erase(dif_process.begin() + i);
                i--;
            }
        }
        //bool background_check=false;
/////////////////////////////////////////////////////////////////////////////////////////////////////////

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }
        else{
            if(tknr.commands[0]->args[0] == "cd"){

                if(tknr.commands[0]->args[1]== "-" ){
                    chdir(prev_dir);
                }
                else{
                    getcwd(prev_dir,1000);
                    chdir((char*)tknr.commands[0]->args[1].c_str());
                }
            continue;
            }
        }
        

        // print out every command token-by-token on individual lines
        // prints to cerr to avoid influencing autograder
        for (auto cmd : tknr.commands) {
            for (auto str : cmd->args) {
                cerr << "|" << str << "| ";
            }
            if (cmd->hasInput()) {
                cerr << "in< " << cmd->in_file << " ";
            }
            if (cmd->hasOutput()) {
                cerr << "out> " << cmd->out_file << " ";
            }
            cerr << endl;
        }

////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        
        
       
        for(size_t i=0; i < tknr.commands.size();i++){
            
/////////////////////////////////////////////////////////////////////////////////////////////////////////           
           // background_check = tknr.commands[i]->isBackground();
/////////////////////////////////////////////////////////////////////////////////////////////////////////

            vector<char*> string_convert;
            for(size_t j=0;j < tknr.commands[i]->args.size(); j++){
                string_convert.push_back((char*)tknr.commands[i]->args[j].c_str());
            }

            // push NULL to the end of the vector (execvp expects NULL as last element)
            string_convert.push_back(NULL);
            // pass the vector's internal array to execvp
            char **comm1 = &string_convert[0];


            int the_pipe[2]; 
            pipe(the_pipe);

            pid_t pid = fork();
            if (pid < 0) {  // error check
                perror("fork");
                exit(2);
            }

            if (pid == 0) {  // if child, exec to run command
                // run single commands with no arguments

                if(i < tknr.commands.size()-1){
                    // In child, redirect output to write end of pipe
                    dup2(the_pipe[1],1);
                    
                    
                }

                //int file;
                if (tknr.commands[i]->hasInput()){
                    int file = open((char*) tknr.commands[i]->in_file.c_str(), O_RDONLY, S_IWUSR | S_IRUSR);
                    dup2(file,0);
                    close(file);
                }
                if (tknr.commands[i]->hasOutput()){
                    int file = open((char*) tknr.commands[i]->out_file.c_str(), O_WRONLY | O_CREAT , S_IWUSR | S_IRUSR);
                    dup2(file,1);
                    close(file);
                }
        
                close(the_pipe[0]);

                execvp(comm1[0], comm1);
                // Close the read end of the pipe on the child side.
                // In child, execute the command
            }
/////////////////////////////////////////////////////////////////////////////////////////////////////////
            else {  // if parent, wait for child to finish

                if(!tknr.commands.at(i)->isBackground()){

                    if (i==tknr.commands.size()-1) {     
                        waitpid(pid, 0, 0);              
                    }   
                waitpid(pid, 0, 0);
                }
                else{
                    dif_process.push_back(pid);
                }
                dup2(the_pipe[0],0);
                close(the_pipe[1]); 
                // if (status > 1) {  // exit if child didn't exec properly
                //     exit(status);
                // }
                // if (i == tknr.commands.size() - 1){
                //     waitpid(pid, &status, 0);
                // }
            }
        }
        dup2(std_in, 0);
        dup2(std_out, 1);
/////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
    return 0;
}