#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "thread.h"
#include <stdlib.h>

using namespace std;

struct reqParam
{
	int requester;
	string fileName;
};

struct request
{
	int disk;
	int track;
};

mutex mutex1;
cv cv1;
cv cv2;
vector<request> storage;
vector<bool> requestMade;
vector<bool> requestDone;
int maxReq;
int threadNum;

void requester(void *a)
{
	
	reqParam *reqinfo = (reqParam *) a;
	string track;
	ifstream diskFile (reqinfo->fileName);
	vector<int> tracks;
	if (diskFile.is_open())
	{
		
		while (getline(diskFile, track))
		{
			int requestTrack = atoi(track.c_str());
			tracks.push_back(requestTrack);
		}
		diskFile.close();
		mutex1.lock();
		for(int i=0; i<tracks.size(); i++)
		{
			request entry;
			entry.disk = reqinfo->requester;
			entry.track = tracks[i];
			while (storage.size() == maxReq || storage.size() == threadNum || requestMade[reqinfo->requester])
			{
				cv1.wait(mutex1);
			}
			storage.push_back(entry);
			cout<<"requester "<<reqinfo->requester<<" track "<<tracks[i]<<endl;
			requestMade[reqinfo->requester] = 1;
			if (i == tracks.size() - 1) requestDone[reqinfo->requester] = 1;
			cv2.signal();
		}
		mutex1.unlock();
	}
	
}

void service(void *a)
{
	vector<string> *arg = (vector<string> *) a;

	for (int i=0; i < arg->size(); i++)
	{
		reqParam *tmpinfo = new reqParam;
		tmpinfo->requester = i;
		tmpinfo->fileName = arg->at(i);
		mutex1.lock();
		requestMade.push_back(0);
		requestDone.push_back(0);
		mutex1.unlock();
		thread((thread_startfunc_t) requester, (void *) tmpinfo);
	}
	int prevTrack = 0;
	mutex1.lock();
	while (!storage.empty() || threadNum!=0)
	{
		while (storage.size() != maxReq && storage.size() != threadNum)
		{
			cv2.wait(mutex1);
		}		
		int index;
		int diff = 0xfffffff;
		for (int i=0; i<storage.size(); i++)
		{
			int tmpdiff = abs(storage[i].track - prevTrack);
			if (tmpdiff < diff)
			{
				diff = tmpdiff;
				index = i;
			}
		}
		prevTrack = storage[index].track;
		cout<<"service requester "<<storage[index].disk<<" track "<<storage[index].track<<endl;
		requestMade[storage[index].disk] = 0;
		if (requestDone[storage[index].disk]) threadNum--;
		storage.erase(storage.begin() + index);
		cv1.broadcast();	
	}
	mutex1.unlock();

}

int main(int argc, char *argv[])
{
	maxReq = atoi(argv[1]);
	vector<string> disk;
	threadNum = argc-2;
	for (int i=0; i < argc-2; i++)
	{
		disk.push_back(argv[i+2]);
	}
	cpu::boot((thread_startfunc_t) service, (void *) &disk, 0);
	return 0;
}