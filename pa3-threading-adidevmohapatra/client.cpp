#include <fstream>
#include <iostream>
#include <thread>
#include <sys/time.h>
#include <sys/wait.h>
#include <vector>


#include "BoundedBuffer.h"
#include "common.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include "FIFORequestChannel.h"


// ecgno to use for datamsgs
#define EGCNO 1

using namespace std;



void patient_thread_function (BoundedBuffer* bufreq, int patient_num, int num_req) {

    // functionality of the patient threads
    datamsg val (patient_num, 0.00, 1);
    for (int i = 0; i < num_req; i++) {
        bufreq->push ((char*) &val, sizeof (datamsg));
        val.seconds += .004;
    }

}

void file_thread_function (string filename, BoundedBuffer* bufreq, int m, FIFORequestChannel* chan) {
    // functionality of the file thread
    filemsg fm(0, 0);

    string fname = filename;
    
    int len = sizeof(filemsg) + (fname.size() + 1);
    char* buf2 = new char[len];
    memcpy(buf2, &fm, sizeof(filemsg));
    strcpy(buf2 + sizeof(filemsg), fname.c_str());

    chan->cwrite(buf2, len);  
    int64_t filesize = 0;
    chan->cread(&filesize, sizeof(int64_t));
    

    FILE* new_file = fopen(const_cast<char*>(("received/" + filename).c_str()), "wb+");
    fseek( new_file , filesize , SEEK_SET );
    fclose(new_file);
    filemsg* file_req = (filemsg*)buf2;
    while (filesize != 0 ){
        //cout << "FILE WRITE" << endl;
        cout << "file size " << filesize << endl;
        if (m > filesize){
            file_req->length=filesize;
        }
        else{
            file_req -> length = m; 
        }   
        //bufreq->push (buf2, len);
        
        bufreq->push (buf2, len);
        file_req -> offset += file_req->length; 
        filesize -= file_req->length;
       
    }
    delete[] buf2;
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    delete chan;

}


struct value{
    int val;
    double val2;

};
void worker_thread_function (BoundedBuffer* bufreq, BoundedBuffer* bufresp, FIFORequestChannel* chan, int m) {
    // functionality of the worker threads

    while (true){
        char request [1000];
        bufreq->pop(request, 1000);
        MESSAGE_TYPE* msg = (MESSAGE_TYPE*) request;
       
        if(*msg==DATA_MSG){

            datamsg* new_req = (datamsg*) request;
            double val3;

            value y;
            y.val = new_req->person;
            

            chan->cwrite(new_req, sizeof(datamsg));
            chan->cread(&val3, sizeof(double));
            y.val2 = val3;

            bufresp->push((char*) &y, sizeof(y));

        }
        else if(*msg==FILE_MSG){
            cout << "Here" << endl;
            filemsg* file=(filemsg*) request;
            string fname = (char *) (file + 1);
            cout << fname << endl;
            char* buffer = new char [m];

            chan->cwrite( request, sizeof(filemsg)+ fname.size()+1);
            chan->cread(buffer,file->length);
            cout << "Read" << endl;
            FILE* new_file=fopen(const_cast<char*>(("received/" + fname).c_str()), "r+b");
            fseek(new_file, file->offset ,SEEK_SET);
            fwrite(buffer, sizeof(buffer[0]) , file->length , new_file);
            fclose(new_file);
            delete[]buffer;
        }
        else  if (*msg == QUIT_MSG){
            chan->cwrite(msg, sizeof(QUIT_MSG));
            delete chan;
            break;
        }
        else{
            exit(0);
        }

    }
    
}
void histogram_thread_function (BoundedBuffer* bufresp, HistogramCollection* coll) {
    // functionality of the histogram threads
    
    while(true){
       value x;
       bufresp->pop((char*) &x, 1000);

       if (x.val > 0 && x.val<16){
           coll->update(x.val,x.val2);
       }
       else{
           break;
       }
    }
}


int main (int argc, char* argv[]) {
    int n = 1000;	// default number of requests per "patient"
    int p = 10;		// number of patients [1,15]
    int w = 100;	// default number of worker threads
	int h = 20;		// default number of histogram threads
    int b = 20;		// default capacity of the request buffer (should be changed)
	int m = MAX_MESSAGE;	// default capacity of the message buffer
	string f = "";	// name of file to be transferred
    
    // read arguments
    int opt;
	while ((opt = getopt(argc, argv, "n:p:w:h:b:m:f:")) != -1) {
		switch (opt) {
			case 'n':
				n = atoi(optarg);
                break;
			case 'p':
				p = atoi(optarg);
                break;
			case 'w':
				w = atoi(optarg);
                break;
			case 'h':
				h = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
                break;
			case 'm':
				m = atoi(optarg);
                break;
			case 'f':
				f = optarg;
                break;
		}
	}
    
	// fork and exec the server
    int pid = fork();
    if (pid == 0) {
        execl("./server", "./server", "-m", (char*) to_string(m).c_str(), nullptr);
    }
    
	// initialize overhead (including the control channel)
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
    BoundedBuffer response_buffer(b);
	HistogramCollection hc;

    //array of producer thread(if data, p elements, if file, 1 element)
    thread* producer = new thread[p];
    //array of FIFO (with elements)
    vector<FIFORequestChannel*> fifo(w);
    //array of worker threads(w elements)
    thread* work = new thread[w];
    //array of histogram threads (if data, h elements, if file, zero element)
    thread* hist = new thread[h];





    // making histograms and adding to collection
    for (int i = 0; i < p; i++) {
        Histogram* h = new Histogram(10, -2.0, 2.0);
        hc.add(h);
    }
	
	// record start time
    struct timeval start, end;
    gettimeofday(&start, 0);
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    
    // for (int i=0; i < w ; ++i ){

    //     MESSAGE_TYPE new_mess = NEWCHANNEL_MSG;
    //     chan->cwrite(&new_mess, sizeof(MESSAGE_TYPE));
    //     char nextbuf[1000];
    //     chan->cread(&nextbuf, 1000);
    //     fifo.at(i) = new FIFORequestChannel(nextbuf, FIFORequestChannel::CLIENT_SIDE);
    // }
    cout << "Test" << endl;
    FIFORequestChannel* newchan;
    thread filecollection;
    if (f !=""){
       // FIFORequestChannel* newchan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
        MESSAGE_TYPE new_mess = NEWCHANNEL_MSG;
        chan->cwrite(&new_mess, sizeof(MESSAGE_TYPE));
        char nextbuf[m];
        chan->cread(&nextbuf, m);
        newchan = new FIFORequestChannel(nextbuf, FIFORequestChannel::CLIENT_SIDE);
        filecollection = thread (file_thread_function,f, &request_buffer, m , newchan);
    }
    else{
        
        for (int i=0; i < p; ++i ){
            producer[i] = (thread (patient_thread_function,&request_buffer, i+1 , n));
        }

        for (int i=0; i < h; ++i){
            hist[i] = (thread (histogram_thread_function,&response_buffer, &hc));
        }
    }
    cout << "Test1" << endl;

    for (int i=0; i < w ; ++i ){
            MESSAGE_TYPE new_mess = NEWCHANNEL_MSG;
            chan->cwrite(&new_mess, sizeof(MESSAGE_TYPE));
            char nextbuf[m];
            chan->cread(&nextbuf, m);
            fifo.at(i) = new FIFORequestChannel(nextbuf, FIFORequestChannel::CLIENT_SIDE);
            work[i] = (thread (worker_thread_function,&request_buffer, &response_buffer, fifo[i], m));
    }

    cout << "Test2" << endl;
    /* create all threads here */
    
    ///////////////////////////////////////////////////////////////////////////////////////////  

	/* join all threads here */
    //iterate over all thread arrays, calling join
    //  -order is important; produces before consumer 
    if (f !=""){

        filecollection.join();
        
    }
    else{   
        for (int i=1; i <= p; ++i ){
            producer[i-1].join();
        }
    }

    

    for (int i=0; i < w ; ++i ){
            MESSAGE_TYPE quit_mess = QUIT_MSG;
            request_buffer.push( (char*) &quit_mess, sizeof(MESSAGE_TYPE) );
    }

    for (int i=0; i < w ; ++i ){
            work[i].join();
    }

    if (f ==""){
        for (int i=0; i < h ; ++i ){
            value v;
            v.val = -1;
            v.val2 = -1;
            response_buffer.push((char*)&v, sizeof(value) );
        }
        for (int i=0; i < h; ++i){
            hist[i].join();
        }
    }
    


    ///////////////////////////////////////////////////////////////////////////////////////////  
	// record end time
    gettimeofday(&end, 0);

    // print the results
	if (f == "") {
		hc.print();
	}




    int secs = ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) / ((int) 1e6);
    int usecs = (int) ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) % ((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;


    //quit and close all channels in FIFO array 
    delete[] work;
    delete[] hist;
    delete[] producer;
	// quit and close control channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!" << endl;
    delete chan;
    // delete newchan;


	// wait for server to exit
	wait(nullptr);
}
