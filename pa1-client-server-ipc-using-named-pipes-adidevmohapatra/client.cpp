/*
    Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
    
    Please include your Name, UIN, and the date below
    Name: Adidev Mohapatra
    UIN: 230007601
    Date: 09/16/2022
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <fstream>
using namespace std;


int main (int argc, char *argv[]) {
    int opt;
    int p = -1;
    double t = -1;
    int e = -1;
    int m = MAX_MESSAGE;
    bool new_chan = false; 
    vector<FIFORequestChannel*>channels;

    
    string filename = "";
    while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
        switch (opt) {
            case 'p':
                p = atoi (optarg);
                break;
            case 't':
                t = atof (optarg);
                break;
            case 'e':
                e = atoi (optarg);
                break;
            case 'f':
                filename = optarg;
                break;
            case 'm':
                m = atoi(optarg);
                break;
            case 'c':
                new_chan = true;
                break;
        }
    }

    // give arguments for the server
    // server needs './server' (path) , '-m', '<val for -m arg>', 'NULL'
    // fork
    // In the child, run execvp using the server arguments.
   	char* args[] = {const_cast<char*>("./server"), const_cast<char*>("-m"), const_cast<char*>(to_string(m).c_str()), NULL};
    pid_t pid = fork();



    if (pid==0){
        execvp(args[0], args);
    }
    else{

        FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);
        channels.push_back(&cont_chan);

        if(new_chan){
            //send newchannel request to the server
            MESSAGE_TYPE nc = NEWCHANNEL_MSG;
            cont_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));
            //create a variable to hold the name
            //cread the response from the server 
            //call the FIFORequestChannel constructor with the name from the server 
            //push the channel back into the vector 
            char* new_chan_value= new char[30];
            cont_chan.cread(new_chan_value, 30);
            channels.push_back(move(new FIFORequestChannel(new_chan_value, FIFORequestChannel::CLIENT_SIDE)));
            delete[] new_chan_value;

        }
        FIFORequestChannel chan = *(channels.back());
        //Single datapoint, only run p,t,e !=1
        //example data point request
        if (p != -1 && t != -1 && e != -1){
            // example data point request
            char buf[MAX_MESSAGE]; // 256
            datamsg x(p, t, e);
            memcpy(buf, &x, sizeof(datamsg));
            chan.cwrite(buf, sizeof(datamsg)); // question
            double reply;
            chan.cread(&reply, sizeof(double)); //answer
            cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
        }
        //Else, if p!=1 request 1000 datapoints.
        else if (p != -1 && t == -1 && e == -1){

            //loop over 1st 1000 lines
            //example data point request
            //write line to recieved/x1/csv
            ofstream my_file("received/x1.csv");
            for (double i = 0; i < 4; i += .004){
            //loop over 1st 1000 lines
            //.004 as the csv file was in increments
                char buf[MAX_MESSAGE]; 
                datamsg x(p, i, 1); //send request for ecg 1
                memcpy(buf, &x, sizeof(datamsg));
                chan.cwrite(buf, sizeof(datamsg));
                //double reply;
                double reply;
                chan.cread(&reply, sizeof(double));
                datamsg x2(p, i, 2); //send request for ecg 2
                memcpy(buf, &x2, sizeof(datamsg));
                chan.cwrite(buf, sizeof(datamsg));
                double reply2;
                chan.cread(&reply2, sizeof(double));


                my_file << i << "," << reply << "," << reply2 << endl ;
            }
            my_file.close();
        }
        //my_file.open();
        // ofstream my_file("received/x1.csv");
        else if(filename.size()!=0){

            // sending a non-sense message, you need to change this
            filemsg fm(0, 0);
            string fname = filename;
            
            int len = sizeof(filemsg) + (fname.size() + 1);
            char* buf2 = new char[len];
            memcpy(buf2, &fm, sizeof(filemsg));
            strcpy(buf2 + sizeof(filemsg), fname.c_str());
            chan.cwrite(buf2, len);  // I want the file length;

            int64_t filesize = 0;
            chan.cread(&filesize, sizeof(int64_t));
            
            char* buf3 = new char[m];//create buffer of size buff capacity(m).

            FILE* new_file = fopen(const_cast<char*>(("received/" + filename).c_str()), "w+b");

            int64_t offset_value =0;
			cout << "Filesize: " << filesize << endl;
            while (filesize != offset_value){
                // loop over the segments in the file filesize/ buff capacity(m).
                //create filemsg instance
                filemsg* file_req = (filemsg*)buf2;

                if (file_req->length > (filesize-offset_value)){
                    file_req->length=filesize-offset_value;
                }
                else{
                    
					file_req -> length = m; //set the length, careful of the last length
                }   
                
                file_req -> offset = offset_value; //set offset in the file
                //send request (buf2)
                chan.cwrite(buf2, len);

                //recieve the response 
                //cread into buf3 length file_req->len
                //write buf3 into file: recieved/filename 
                chan.cread(buf3, file_req->length);
				// cout << "Nullptr? " << (new_file == nullptr) << endl;
                fwrite(buf3, sizeof(buf3[0]), file_req->length, new_file);
                offset_value += file_req->length;
            }
            fclose(new_file);
        }

        //close and delete the new channel
        if (new_chan){
            //do your close and delete 
            MESSAGE_TYPE delete_chan = QUIT_MSG;
            chan.cwrite(&delete_chan, sizeof(MESSAGE_TYPE));
            delete channels.at(1);


        }


        // closing the channel    
        MESSAGE_TYPE m = QUIT_MSG;
        cont_chan.cwrite(&m, sizeof(MESSAGE_TYPE));
    }
    wait(NULL);
}