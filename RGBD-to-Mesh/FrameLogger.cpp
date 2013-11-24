#include "FrameLogger.h"


FrameLogger::FrameLogger(void)
{
}


FrameLogger::~FrameLogger(void)
{

}

void FrameLogger::record(string outputDirectory)
{
	//Open log file
	ofstream logfile (outputDirectory+"\\log.xml");

	if(logfile.is_open())
	{
		//Loop while still recording or still has frames to save. Everything up to the point stopRecording is called WILL be saved
		mQueueGuard.lock();
		bool queueIsEmpty = mFrameQueue.empty();
		mQueueGuard.unlock();

		//Open log
		int xRes = mDevice->getColorResolutionX();
		int yRes = mDevice->getColorResolutionY();

		//Root node
		logfile << "<device xresolution=\"" << xRes << "\" yresolution=\"" << yRes << "\">" << endl;

		int frameCount = 0;
		while(mIsRecording || !queueIsEmpty)
		{
			//
			if(!queueIsEmpty)
			{
				frameCount++;
				mQueueGuard.lock();
				RGBDFramePtr localFrame = mFrameQueue.front();
				mFrameQueue.pop();
				mQueueGuard.unlock();

				//Print id
				logfile << "<frame id=\"" << frameCount << "\"";
				if(localFrame->hasColor()){
					timestamp time = localFrame->getColorTimestamp();
					logfile << " colorTimestamp=\"" << time << "\"";
				}

				if(localFrame->hasDepth()){
					timestamp time = localFrame->getDepthTimestamp();
					logfile << " depthTimestamp=\"" << time << "\"";
				}

				//Close tag
				logfile << "/>";

				//Save frames
				ostringstream s;
				s << outputDirectory << "\\" << frameCount;
				saveRGBDFrameImagesToFiles(s.str(), localFrame);
			}

			mQueueGuard.lock();
			queueIsEmpty = mFrameQueue.empty();
			mQueueGuard.unlock();
		}



		//Close log file
		logfile << "</device>" << endl;
		logfile.close();
	}
	mIsRecording = false;
}

//Create the output directory. Returns false if directory could not be created or is not empty
bool FrameLogger::makeOutputDirectory()
{
	if(isDirectoryEmpty(mOutputDirectory))
	{
		return makeDir(mOutputDirectory);
	}
	return false;
}


//Returns true if successfully created
bool FrameLogger::startRecording(RGBDDevice* device)
{

	if(!makeOutputDirectory())
		return false;
	if(!isDirectoryEmpty(mOutputDirectory))
		return false;


	mQueueGuard.lock();
	mFrameQueue = queue<RGBDFramePtr>();//Clear queue
	mQueueGuard.unlock();

	//Register listener
	mDevice = device;
	mDevice->addNewRGBDFrameListener(this);

	//Launch thread
	mIsRecording = true;
	mLoggerThread = std::thread(&FrameLogger::record, this, mOutputDirectory);

	return true;
}

void FrameLogger::stopRecording()
{
	mDevice->removeNewRGBDFrameListener(this);
	mIsRecording = false;
	mLoggerThread.join();//Wait for thread to die (finish saving)
	mDevice = NULL;
}

void FrameLogger::onNewRGBDFrame(RGBDFramePtr frame)
{
	if(mIsRecording)
	{
		mQueueGuard.lock();
		mFrameQueue.push(frame);
		mQueueGuard.unlock();
	}
}
