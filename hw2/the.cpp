#include <iostream>
#include "monitor.h"
#include <vector>
//extern "C" {
	#include "writeOutput.h"
//}
extern "C" {
	#include <unistd.h>
}

using namespace std;
int exitedSmelters = 0;
int exitedFoundries = 0;
class Miner{
	private:
		int id;
		int capacity;
		OreType oretype;
		int totalOre;
		int interval;
		int currentCount;
		bool isActive;
		bool reserved;
	public:
		Miner(int id, int totalOre, int oretype, int capacity, int interval){
			this->id = id;
			this->capacity = capacity;
			this->oretype =(OreType) oretype;
			this->totalOre = totalOre;
			this->interval = interval;
			this->currentCount = 0;
			this->isActive = true;
			this->reserved = false;
		};
		~Miner();
		
		int getID(){
			return this->id;
		}

		int getCapacity(){
			return this->capacity;
		}

		OreType getOreType(){
			return this->oretype;
		}

		int getTotalOre(){
			return this->totalOre;
		}

		void decrementTotalOre(){
			this->totalOre--;
		}

		int getInterval(){
			return this->interval; 
		}

		int getCurrentCount(){
			return this->currentCount;
		}

		void incrementCurrentCount(){
			this->currentCount++;
		}

		void decrementCurrentCount(){
			this->currentCount--;
		}

		bool isActiveStatus(){
			return this->isActive;
		}

		void setInactive(){
			this->isActive = false;
		}

		bool isFull(){
			return this->capacity==this->currentCount;
		}

		bool hasTransporter(){
			return this->reserved;
		}

		void toggleHasTransporter(){
			this->reserved = !this->reserved;
		}

};

class Transporter{
	private:
		int interval;
		int id;
		OreType* carry;
	public:
		Transporter(int id, OreType* carry, int interval){
			this->id = id;
			this->carry = carry;
			this->interval = interval;
		}
		~Transporter();	

		int getID(){
			return this->id;
		}

		int getInterval(){
			return this->interval;
		}

		OreType* getCarry(){
			return this->carry;
		}

		void setCarry(OreType type){
			*(this->carry) = type;
		}
};

class Foundry{
	private:
		int id, loadingCapacity ,ironCount, coalCount, totalProduce, interval;
		bool active, reserved;
	public:
		Foundry(int id, int loadingCapacity, int interval){
			this->id = id;
			this->interval = interval;
			this->loadingCapacity = loadingCapacity;
			this->totalProduce = 0;
			this->ironCount = 0;
			this->coalCount = 0;
			this->active = true;
			this->reserved = false;
		}
		~Foundry();
		
		int getID(){
			return this->id;
		}

		int getInterval(){
			return this->interval;
		}

		int getLoadingCapacity(){
			return this->loadingCapacity;
		}

		int getIronCount(){
			return this->ironCount;
		}

		void incrementIronCount(){
			this->ironCount++;
		}

		void decrementIronCount(){
			this->ironCount--;
		}

		int getCoalCount(){
			return this->coalCount;
		}

		void incrementCoalCount(){
			this->coalCount++;
		}

		void decrementCoalCount(){
			this->coalCount--;
		}

		int getTotalProduced(){
			return this->totalProduce;
		}

		void incrementTotalProduce(){
			this->totalProduce++;
		}

		bool isActive(){
			return this->active;
		}

		void setInactive(){
			this->active = false;
		}

		bool isEmpty(){
			return this->ironCount + this->coalCount == 0;
		}

		bool hasTransporter(){
			return this->reserved;
		}

		void toggleHasTransporter(){
			this->reserved = !this->reserved;
		}		

		bool isFull(){
			return this->loadingCapacity == this->ironCount + this->coalCount;
		}
};

class Smelter{
	private:
		int id, loadingCapacity, oreCount ,totalProduce, interval;
		OreType oretype;
		bool active, reserved;
	public:
		Smelter(int id, OreType oretype, int loadingCapacity, int interval){
			this->id = id;
			this->interval = interval;
			this->loadingCapacity = loadingCapacity;
			this->totalProduce = 0;
			this->oreCount = 0;
			this->oretype = oretype;
			this->active = true;
			this->reserved = false;
		}
		~Smelter();
		
		int getID(){
			return this->id;
		}

		OreType getOreType(){
			return this->oretype;
		}

		int getInterval(){
			return this->interval;
		}

		int getLoadingCapacity(){
			return this->loadingCapacity;
		}

		int getOreCount(){
			return this->oreCount;
		}

		void incrementOreCount(){
			this->oreCount++;
		}

		void decrementOreCount(){
			this->oreCount--;
		}

		int getTotalProduced(){
			return this->totalProduce;
		}

		void incrementTotalProduce(){
			this->totalProduce++;
		}

		void setInactive(){
			this->active = false;
		}

		bool isActive(){
			return this->active;
		}

		bool isEmpty(){
			return this->oreCount == 0;
		}

		bool hasTransporter(){
			return this->reserved;
		}

		void toggleHasTransporter(){
			this->reserved = !this->reserved;
		}

		bool isFull(){
			return this->loadingCapacity==this->oreCount;
		}
};

vector<Miner*> minerVector;
vector<Foundry *> foundryVector;
vector<Smelter *> smelterVector;
int lastLoad=-1;
int toLoad = -1;

class MineSimulation: public Monitor{
	private:
		bool minersFinished = false;
		Condition canProduce, minerProduced;
		Condition startIronSmelters, startCopperSmelters, startFoundry, ironPlace, copperPlace, coalPlace;

	public:
		MineSimulation():canProduce(this), minerProduced(this), startCopperSmelters(this), startIronSmelters(this), startFoundry(this), ironPlace(this), copperPlace(this), coalPlace(this){
		}
	    void WaitCanProduce(Miner* miner){
			__synchronized__;
			while(miner->isFull())
				canProduce.wait();
	    }

	    void MinerProduced(Miner* miner) {
	        __synchronized__;
	        miner->incrementCurrentCount();
	        minerProduced.notifyAll();
	    }

	    void MinerStopped(Miner* miner){
			__synchronized__;
			if(miner==NULL){
				minersFinished = true;
				minerProduced.notifyAll();
			}else{
				miner->setInactive();
				bool allStopped=true;
				for(int i=0 ; i<minerVector.size() ; i++){
		    		if(minerVector[i]->isActiveStatus() || minerVector[i]->getCurrentCount())
						allStopped = false;
				}
				if(allStopped){
					minersFinished = true;
					minerProduced.notifyAll();
				}	
			}
			
	    }

	    bool checkMinerStatus(){
	    	__synchronized__;
	    	for(int i=0 ; i<minerVector.size() ; i++){
	    		if(minerVector[i]->isActiveStatus() || minerVector[i]->getCurrentCount())
    				return true;		
	    	}
	    	return false;
	    }

	    Miner* WaitNextLoad(){
			__synchronized__;

			bool shouldReturnNULL=true;
			for(int i=0 ; i<minerVector.size() ; i++){
	    		if(minerVector[i]->isActiveStatus() || minerVector[i]->getCurrentCount())
	
	    			shouldReturnNULL=false;
	    	}
			if(shouldReturnNULL){
				return NULL;
			}

			toLoad++;
			if(toLoad>=minerVector.size()){
				toLoad-=minerVector.size();
			}
			while(1){
				if(minerVector[toLoad]<0){
					return NULL;	
				} 
				if(minerVector[toLoad]->getCurrentCount()>0 && !minerVector[toLoad]->hasTransporter()){
					minerVector[toLoad]->toggleHasTransporter();
					lastLoad = toLoad;
					return minerVector[toLoad];
				}else if(toLoad==lastLoad){
					minerProduced.wait();
					toLoad++;
					if(toLoad>=minerVector.size()){
						toLoad-=minerVector.size();
					}
					if(this->minersFinished){
						return NULL;
					}
				}else{
					toLoad++;
					if(toLoad>=minerVector.size()){
						toLoad-=minerVector.size();
					}
				}
			}
			return NULL;	    
		}

		void SmelterProduced(Smelter* smelter){
			__synchronized__;
			smelter->incrementTotalProduce();
			smelter->decrementOreCount();
			smelter->decrementOreCount();
			if(smelter->getOreType()==IRON){
				ironPlace.notifyAll();
			}
			else{
				copperPlace.notifyAll();
			}
			return;
		}

		void SmelterStopped(Smelter* smelter){
			__synchronized__;
			smelter->setInactive();
		}

		void FoundryProduced(Foundry* foundry){
			__synchronized__;
			
			foundry->incrementTotalProduce();
			foundry->decrementCoalCount();
			foundry->decrementIronCount();
			ironPlace.notifyAll();
			coalPlace.notifyAll();
		}

		void FoundryStopped(Foundry* foundry){
			__synchronized__;
			foundry->setInactive();
		}


		void WaitProducer(Miner* miner, Smelter*& smelter, Foundry*& foundry){
			__synchronized__;
			OreType oretype = miner->getOreType();
			bool foundProducer = false;
			if(oretype == COPPER){ // carry to smelter
				while(1){
					for(int i=0 ; i<smelterVector.size() ; i++){
						if(!smelterVector[i]->isActive()){
							continue;
						}
						if(smelterVector[i]->isFull())
							continue;
						if(smelterVector[i]->getOreType()!=COPPER){
							continue;
						}
						if(!smelterVector[i]->isEmpty() && !smelterVector[i]->hasTransporter() && smelterVector[i]->getOreCount() == 1){
							smelter = smelterVector[i];
							foundry = NULL;
							foundProducer = true;
							break;
						}
					}

					for(int i=0 ; i<smelterVector.size() ; i++){
						if(foundProducer == true)
							break;
						if(!smelterVector[i]->isActive()){
							continue;
						}
						if(smelterVector[i]->isFull())
							continue;
						if(smelterVector[i]->getOreType()!=COPPER){
							continue;
						}
						if(smelterVector[i]->isEmpty() && !smelterVector[i]->hasTransporter()){
							smelter = smelterVector[i];
							foundry = NULL;
							foundProducer = true;
							break;
						}
					}

					for(int i=0 ; i<smelterVector.size() ; i++){
						if(foundProducer == true)
							break;
						if(!smelterVector[i]->isActive()){
							continue;
						}
						if(smelterVector[i]->isFull())
							continue;
						if(smelterVector[i]->getOreType()!=COPPER){
							continue;
						}
						if(!smelterVector[i]->hasTransporter()){
							smelter = smelterVector[i];
							foundry = NULL;
							foundProducer = true;
							break;
						}
					}

					if(!foundProducer){
						copperPlace.wait();
					}else{
						smelter->toggleHasTransporter();
						break;
					}

				}
			}else if(oretype == IRON){ // carry to foundry or smelter
				while(1){
					for(int i=0 ; i<foundryVector.size() ; i++){
						if(!foundryVector[i]->isActive()){
							continue;
						}
						
						if(foundryVector[i]->isFull())
							continue;
						if(!foundryVector[i]->isEmpty() && !foundryVector[i]->hasTransporter() && foundryVector[i]->getIronCount() == 0 && foundryVector[i]->getCoalCount() >=1){
							foundry = foundryVector[i];
							smelter = NULL;
							foundProducer = true;
							break;
						}
					}

					for(int i=0 ; i<smelterVector.size() ; i++){
						if(!smelterVector[i]->isActive()){
							continue;
						}
						if(smelterVector[i]->isFull())
							continue;
						if(smelterVector[i]->getOreType()!=IRON){
							continue;
						}
						if(!smelterVector[i]->isEmpty() && !smelterVector[i]->hasTransporter() && smelterVector[i]->getOreCount() == 1){
							smelter = smelterVector[i];
							foundry = NULL;
							foundProducer = true;
							break;
						}
					}

					for(int i=0 ; i<foundryVector.size() ; i++){
						if(foundProducer == true)
							break;
						if(!foundryVector[i]->isActive()){
							continue;
						}
						if(foundryVector[i]->isFull())
							continue;
						if(foundryVector[i]->isEmpty() && !foundryVector[i]->hasTransporter()){
							foundry = foundryVector[i];
							smelter = NULL;
							foundProducer = true;

							break;
						}
					}

					for(int i=0 ; i<smelterVector.size() ; i++){
						if(foundProducer == true)
							break;
						if(!smelterVector[i]->isActive()){
							continue;
						}
						if(smelterVector[i]->isFull())
							continue;
						if(smelterVector[i]->getOreType()!=IRON){
							continue;
						}
						if(smelterVector[i]->isEmpty() && !smelterVector[i]->hasTransporter()){
							smelter = smelterVector[i];
							foundry = NULL;
							foundProducer = true;
							break;
						}
					}

					for(int i=0 ; i<foundryVector.size() ; i++){
						if(foundProducer){
							break;
						}
						if(!foundryVector[i]->isActive())
							continue;
						if(foundryVector[i]->isFull())
							continue;
						if(!foundryVector[i]->hasTransporter()){
							foundry = foundryVector[i];
							smelter = NULL;
							foundProducer = true;
							break;
						}
					}

					for(int i=0 ; i<smelterVector.size() ; i++){
						if(foundProducer == true)
							break;
						if(!smelterVector[i]->isActive()){
							continue;
						}
						if(smelterVector[i]->isFull())
							continue;
						if(smelterVector[i]->getOreType()!=IRON){
							continue;
						}
						if(!smelterVector[i]->hasTransporter()){
							smelter = smelterVector[i];
							foundry = NULL;
							foundProducer = true;
							break;
						}
					}

					if(!foundProducer){
						ironPlace.wait();
					}else{
						if(smelter){
							smelter->toggleHasTransporter();
						}else{
							foundry->toggleHasTransporter();
						}
						break;
					}
				}

			}else{ // carry to foundry
				while(1){
					for(int i=0 ; i<foundryVector.size() ; i++){
						if(!foundryVector[i]->isActive()){
							continue;
						}
						if(foundryVector[i]->isFull())
							continue;
						if(!foundryVector[i]->isEmpty() && !foundryVector[i]->hasTransporter() && foundryVector[i]->getCoalCount() == 0 && foundryVector[i]->getIronCount() >=1){
							foundry = foundryVector[i];
							smelter = NULL;
							foundProducer = true;
							break;
						}
					}

					for(int i=0 ; i<foundryVector.size() ; i++){
						if(foundProducer == true)
							break;
						if(!foundryVector[i]->isActive()){
							continue;
						}
						if(foundryVector[i]->isFull())
							continue;
						if(foundryVector[i]->isEmpty() && !foundryVector[i]->hasTransporter()){
							foundry = foundryVector[i];
							smelter = NULL;
							foundProducer = true;
							break;
						}
					}

					for(int i=0 ; i<foundryVector.size() ; i++){
						if(foundProducer){
							break;
						}
						if(!foundryVector[i]->isActive())
							continue;
						if(foundryVector[i]->isFull())
							continue;
						if(!foundryVector[i]->hasTransporter()){
							foundry = foundryVector[i];
							smelter = NULL;
							foundProducer = true;
							break;
						}
					}

					if(!foundProducer){
						coalPlace.wait();
					}else{
						foundry->toggleHasTransporter();
						break;
					}

				}
			}
			return;
		}

		int WaitUntilTwoOres(Smelter* smelter){
			__synchronized__;
			int noTimeout=0;
			while(smelter->getOreCount()<2 && noTimeout==0){
				if(smelter->getOreType()==IRON){
					noTimeout=startIronSmelters.timedwait();
				}else if(smelter->getOreType()==COPPER){
					noTimeout=startCopperSmelters.timedwait();
				}
			}
			return noTimeout;
		}

		int WaitForOneIronOneCoal(Foundry* foundry){
			__synchronized__;
			int noTimeout=0;
			while((foundry->getIronCount()==0 || foundry->getCoalCount()==0) && noTimeout==0){
				noTimeout=startFoundry.timedwait();
			}

			return noTimeout;

		}

	    void Loaded(Miner* miner){
			__synchronized__;
			bool finished=true;
			for(int i=0 ; i<minerVector.size() ; i++){
	    		if(minerVector[i]->isActiveStatus() || minerVector[i]->getCurrentCount())
	    			finished=false;
	    	}
	    	if(finished){
	    		this->minersFinished = true;
	    	}else{
				canProduce.notifyAll();
	    	}
			return;
	    }

	    void unloadOreToSmelter(Miner* miner, Smelter* smelter){
	    	__synchronized__;
			if(miner->getOreType()==IRON){
				startIronSmelters.notifyAll();
			}else if(miner->getOreType()==COPPER){
				startCopperSmelters.notifyAll();
			}
			return;
	    }

	    void unloadOreToFoundry(Miner* miner, Foundry* foundry){
	    	__synchronized__;
			if(miner->getOreType()==IRON){
				startFoundry.notifyAll();
			}else if(miner->getOreType()==COAL){
				startFoundry.notifyAll();
			}
			return;
	    }

	    void mt1(Miner* miner, Transporter* transporter){
	    	__synchronized__;
	    	MinerInfo* info = new MinerInfo();
			TransporterInfo* tinfo = new TransporterInfo();
			FillMinerInfo(info, miner->getID(), miner->getOreType(), miner->getCapacity(), miner->getCurrentCount());
			FillTransporterInfo(tinfo,transporter->getID(), NULL);
			WriteOutput(info, tinfo, NULL, NULL,TRANSPORTER_TRAVEL);
			transporter->setCarry(miner->getOreType());	

	    }

	    void mt2(Miner* miner, Transporter* transporter){
	    	__synchronized__;
	    	MinerInfo* info = new MinerInfo();
			TransporterInfo* tinfo = new TransporterInfo();
			miner->decrementCurrentCount();
			FillMinerInfo(info, miner->getID(), miner->getOreType(), miner->getCapacity(), miner->getCurrentCount());
			FillTransporterInfo(tinfo,transporter->getID(),transporter->getCarry());
			WriteOutput(info, tinfo, NULL, NULL,TRANSPORTER_TAKE_ORE);
			miner->toggleHasTransporter();
	    }

	    void mt3(Smelter* smelter, Foundry* foundry, Transporter* transporter, int type){
	    	__synchronized__;
  			TransporterInfo *tinfo = new TransporterInfo();
  			SmelterInfo *sinfo = new SmelterInfo();
  			FoundryInfo *finfo = new FoundryInfo();

	    	if(foundry && !type){
	      		FillFoundryInfo(finfo, foundry->getID(), foundry->getLoadingCapacity(), foundry->getIronCount(), foundry->getCoalCount(), foundry->getTotalProduced());
      			FillTransporterInfo(tinfo, transporter->getID(), transporter->getCarry());
      			WriteOutput(NULL, tinfo, NULL, finfo,TRANSPORTER_TRAVEL);
	    	}else if(foundry && type){
				if(*(transporter->getCarry())==0){ 
					foundry->incrementIronCount();
        		}else{ 
        			foundry->incrementCoalCount();
        		}
    			FillFoundryInfo(finfo, foundry->getID(), foundry->getLoadingCapacity(), foundry->getIronCount(), foundry->getCoalCount(), foundry->getTotalProduced());
  				FillTransporterInfo(tinfo, transporter->getID(), transporter->getCarry());
  				WriteOutput(NULL, tinfo, NULL, finfo,TRANSPORTER_DROP_ORE);
  				foundry->toggleHasTransporter();
	    	}else if(smelter && !type){
				FillSmelterInfo(sinfo,smelter->getID(), *(transporter->getCarry()), smelter->getLoadingCapacity(), smelter->getOreCount(), smelter->getTotalProduced());
	      		FillTransporterInfo(tinfo, transporter->getID(), transporter->getCarry());
      			WriteOutput(NULL, tinfo, sinfo, NULL,TRANSPORTER_TRAVEL);
	    	}else{
	    		smelter->incrementOreCount();
	    		FillSmelterInfo(sinfo, smelter->getID(), *(transporter->getCarry()), smelter->getLoadingCapacity(), smelter->getOreCount(), smelter->getTotalProduced());
				FillTransporterInfo(tinfo, transporter->getID(), transporter->getCarry());
				WriteOutput(NULL, tinfo, sinfo, NULL,TRANSPORTER_DROP_ORE);
	    		smelter->toggleHasTransporter();
	    	}
	    }

	    void mt4(Smelter* smelter, int type){
    	   __synchronized__;
			SmelterInfo *info = new SmelterInfo();
			if(type==0){
				info = new SmelterInfo();
				FillSmelterInfo(info, smelter->getID(), smelter->getOreType(), smelter->getLoadingCapacity(), smelter->getOreCount(), smelter->getTotalProduced());
				WriteOutput(NULL, NULL, info, NULL, SMELTER_STARTED);
			}else if(type==1){
				info = new SmelterInfo();
				FillSmelterInfo(info, smelter->getID(), smelter->getOreType(), smelter->getLoadingCapacity(), smelter->getOreCount(), smelter->getTotalProduced());
    		  	WriteOutput(NULL, NULL, info, NULL, SMELTER_FINISHED);
			}else if(type==2){
				info = new SmelterInfo();
				FillSmelterInfo(info, smelter->getID(), smelter->getOreType(), smelter->getLoadingCapacity(), smelter->getOreCount(), smelter->getTotalProduced());
		      	WriteOutput(NULL, NULL, info, NULL, SMELTER_STOPPED);
			}
		}

		void mt5(Foundry* foundry, int type){
			__synchronized__;
			FoundryInfo *info = new FoundryInfo();
			if(type==0){
				info = new FoundryInfo();
				FillFoundryInfo(info, foundry->getID(), foundry->getLoadingCapacity(), foundry->getIronCount(), foundry->getCoalCount(), foundry->getTotalProduced());
				WriteOutput(NULL, NULL, NULL, info, FOUNDRY_STARTED);
			}else if(type==1){
				info = new FoundryInfo();
				FillFoundryInfo(info, foundry->getID(), foundry->getLoadingCapacity(), foundry->getIronCount(), foundry->getCoalCount(), foundry->getTotalProduced());
				WriteOutput(NULL, NULL, NULL, info, FOUNDRY_FINISHED);
			}else{
				info = new FoundryInfo();
				FillFoundryInfo(info, foundry->getID(), foundry->getLoadingCapacity(), foundry->getIronCount(), foundry->getCoalCount(), foundry->getTotalProduced());
				WriteOutput(NULL, NULL, NULL, info, FOUNDRY_STOPPED);
			}
		}

};

MineSimulation msMonitor; 

void* minerThreadRoutine(void* args){
	Miner* miner=(Miner*)args;
	MinerInfo* info = new MinerInfo();
	FillMinerInfo(info, miner->getID(), miner->getOreType(), miner->getCapacity(),miner->getCurrentCount());
	WriteOutput(info, NULL, NULL, NULL, MINER_CREATED);		
	while (miner->getTotalOre()){
		msMonitor.WaitCanProduce(miner);
		info = new MinerInfo();
    	FillMinerInfo(info, miner->getID(), miner->getOreType(), miner->getCapacity(),miner->getCurrentCount());
		WriteOutput(info, NULL, NULL, NULL, MINER_STARTED);			
		usleep(miner->getInterval() - (miner->getInterval()*0.01) + (rand()%(int)(miner->getInterval()*0.02)));
		msMonitor.MinerProduced(miner);
		info = new MinerInfo();
    	FillMinerInfo(info, miner->getID(), miner->getOreType(), miner->getCapacity(),miner->getCurrentCount());
    	WriteOutput(info, NULL, NULL, NULL, MINER_FINISHED);			
    	usleep(miner->getInterval() - (miner->getInterval()*0.01) + (rand()%(int)(miner->getInterval()*0.02))) ;
		miner->decrementTotalOre();
	}
	msMonitor.MinerStopped(miner);
	info = new MinerInfo();
	FillMinerInfo(info, miner->getID(), miner->getOreType(), miner->getCapacity(),miner->getCurrentCount());
	WriteOutput(info, NULL, NULL, NULL, MINER_STOPPED);		
}

void* transporterThreadRoutine(void* args){
	Transporter* transporter=(Transporter*)args;
	TransporterInfo* transporterInfo = new TransporterInfo();
	FillTransporterInfo(transporterInfo,transporter->getID() ,NULL);
	WriteOutput(NULL, transporterInfo, NULL, NULL,TRANSPORTER_CREATED);
	Miner* miner; 
	Smelter* smelter;
	Foundry* foundry;
	while(msMonitor.checkMinerStatus()){
		miner = msMonitor.WaitNextLoad();
		if(miner==NULL){
			break;
		}
		msMonitor.mt1(miner, transporter);
		usleep(transporter->getInterval() - (transporter->getInterval()*0.01) + (rand()%(int)(transporter->getInterval()*0.02)));
		msMonitor.mt2(miner,transporter);		
		usleep(transporter->getInterval() - (transporter->getInterval()*0.01) + (rand()%(int)(transporter->getInterval()*0.02)));
		msMonitor.Loaded(miner);
		msMonitor.WaitProducer(miner, smelter, foundry);
		if(smelter){
			msMonitor.mt3(smelter, NULL, transporter, 0);
			usleep(transporter->getInterval() - (transporter->getInterval()*0.01) + (rand()%(int)(transporter->getInterval()*0.02)));
			msMonitor.mt3(smelter, NULL, transporter, 1);
			usleep(transporter->getInterval() - (transporter->getInterval()*0.01) + (rand()%(int)(transporter->getInterval()*0.02)));
			msMonitor.unloadOreToSmelter(miner, smelter);	
		}else{
			msMonitor.mt3(NULL, foundry, transporter, 0);
			usleep(transporter->getInterval() - (transporter->getInterval()*0.01) + (rand()%(int)(transporter->getInterval()*0.02)));
			msMonitor.mt3(NULL, foundry, transporter, 1);
			usleep(transporter->getInterval() - (transporter->getInterval()*0.01) + (rand()%(int)(transporter->getInterval()*0.02)));
			msMonitor.unloadOreToFoundry(miner, foundry);
		}
	}
	msMonitor.MinerStopped(NULL);
	transporterInfo = new TransporterInfo();
	transporter->setCarry(IRON);
	FillTransporterInfo(transporterInfo,transporter->getID(), transporter->getCarry());
	WriteOutput(NULL, transporterInfo, NULL, NULL,TRANSPORTER_STOPPED);
}

void* smelterThreadRoutine(void* args){
	Smelter* smelter=(Smelter*)args;
	SmelterInfo* info = new SmelterInfo();
	int timeout;
	FillSmelterInfo(info, smelter->getID(), smelter->getOreType(), smelter->getLoadingCapacity(), smelter->getOreCount(), smelter->getTotalProduced());
	WriteOutput(NULL, NULL, info, NULL, SMELTER_CREATED);

	while(1){
		timeout = msMonitor.WaitUntilTwoOres(smelter);
		if(timeout!=0)
			break;
		msMonitor.mt4(smelter, 0);
		usleep(smelter->getInterval() - (smelter->getInterval()*0.01) + (rand()%(int)(smelter->getInterval()*0.02)));
		msMonitor.SmelterProduced(smelter);
		msMonitor.mt4(smelter, 1);
	}
	msMonitor.SmelterStopped(smelter);
	msMonitor.mt4(smelter, 2);
	exitedSmelters++;
	if(exitedSmelters==smelterVector.size() && exitedFoundries==foundryVector.size()){
		exit(0);
	}
}

void* foundryThreadRoutine(void* args){
	Foundry* foundry = (Foundry*)args;
	int timeout;
	FoundryInfo *info = new FoundryInfo();
	FillFoundryInfo(info, foundry->getID(), foundry->getLoadingCapacity(), foundry->getIronCount(), foundry->getCoalCount(), foundry->getTotalProduced());
	WriteOutput(NULL, NULL, NULL, info, FOUNDRY_CREATED);
	while(1){
		timeout = msMonitor.WaitForOneIronOneCoal(foundry);
		if(timeout!=0)
			break;
		msMonitor.mt5(foundry, 0);
		usleep(foundry->getInterval() - (foundry->getInterval()*0.01) + (rand()%(int)(foundry->getInterval()*0.02))) ;
		msMonitor.FoundryProduced(foundry);
		msMonitor.mt5(foundry, 1);

	}
	msMonitor.FoundryStopped(foundry);
	msMonitor.mt5(foundry, 2);
	exitedFoundries++;
	if(exitedSmelters==smelterVector.size() && exitedFoundries==foundryVector.size()){
		exit(0);
	}
}

int main(){
	int minerCount, transporterCount, smelterCount, foundryCount;

	cin >> minerCount;
	int minerProperties[minerCount][4];
	for(int i=0 ; i<minerCount ; i++) 
		cin >> minerProperties[i][0] >> minerProperties[i][1] >> minerProperties[i][2] >> minerProperties[i][3];
	
	cin >> transporterCount;
	int transporterProperties[transporterCount][1];
	for(int i=0 ; i<transporterCount ; i++) 
		cin >> transporterProperties[i][0];

	cin >> smelterCount;
	int smelterProperties[smelterCount][3];
	for(int i=0 ; i<smelterCount ; i++) 
		cin >> smelterProperties[i][0] >> smelterProperties[i][1] >> smelterProperties[i][2];
	
	cin >> foundryCount;
	int foundryProperties[foundryCount][2];
	for(int i=0 ; i<foundryCount ; i++) 
		cin >> foundryProperties[i][0] >> foundryProperties[i][1];
	

	InitWriteOutput();

	pthread_t minerthreads[minerCount];
	for(int i=0 ; i<minerCount ; i++){
		Miner* miner = new Miner(i+1, minerProperties[i][3], minerProperties[i][2], minerProperties[i][1], minerProperties[i][0]);
		minerVector.push_back(miner);
		lastLoad++;
		pthread_create(minerthreads+i, NULL, minerThreadRoutine ,(void*) miner);
	}

  	pthread_t transporterthreads[transporterCount];
  	for(int i=0 ; i<transporterCount ; i++){
  		OreType* carry = new OreType();
		Transporter* transporter=new Transporter(i+1, carry, transporterProperties[i][0]);
		pthread_create(transporterthreads+i, NULL, transporterThreadRoutine,(void*) transporter);
	}

	pthread_t smelterthreads[smelterCount];
	for(int i=0 ; i<smelterCount ; i++){
		Smelter* smelter = new Smelter(i+1, (OreType)smelterProperties[i][2] ,smelterProperties[i][1] ,smelterProperties[i][0]);
		smelterVector.push_back(smelter);
		pthread_create(smelterthreads+i, NULL, smelterThreadRoutine, (void*) smelter);
	}

	pthread_t foundrythreads[foundryCount];
	for(int i=0 ; i<foundryCount ; i++){
		Foundry* foundry = new Foundry(i+1, foundryProperties[i][1], foundryProperties[i][0]);
		foundryVector.push_back(foundry);
		pthread_create(foundrythreads+i, NULL, foundryThreadRoutine, (void*) foundry);
	}


	for(int i=0 ; i<minerCount ; i++){
	    pthread_join(minerthreads[i],NULL);
  	}	

	for(int i=0 ; i<transporterCount ; i++){
	    pthread_join(transporterthreads[i],NULL);
  	}	

	for(int i=0 ; i<smelterCount ; i++){
	    pthread_join(smelterthreads[i],NULL);
  	}

	for(int i=0 ; i<foundryCount ; i++){
	    pthread_join(foundrythreads[i],NULL);
  	}
	return 0;

}