#include "cpmfsys.h"
#include <ctype.h>

bool freeList[256];

DirStructType *mkDirStruct(int index,uint8_t *e)
{
	DirStructType *dstPointer = malloc(sizeof *dstPointer);
	int firstByteExtent = index*32;
	dstPointer->status = e[firstByteExtent];
	for (int i=1;i<9;i++)
	{
		dstPointer->name[i-1]= e[firstByteExtent+i];
	}
	for(int j=1;j<4;j++)
	{
		dstPointer->extension[j-1]=e[firstByteExtent+8+j];
	}
	dstPointer->XL=e[firstByteExtent+12];
	dstPointer->BC=e[firstByteExtent+13];
	dstPointer->XH=e[firstByteExtent+14];
	dstPointer->RC=e[firstByteExtent+15];
	for (int k=1;k<17;k++)
	{
		dstPointer->blocks[k-1]= e[firstByteExtent+15+k];
	}
	return dstPointer;
}


void writeDirStruct(DirStructType *d, uint8_t index, uint8_t *e)
{
	int firstByteExtent = index*32;
	e[firstByteExtent]=d->status;
	for(int i=1;i<9;i++)
	{
		e[firstByteExtent+i]=d->name[i-1];
	}
	for(int j=1;j<4;j++)
	{
		e[firstByteExtent+8+j]=d->extension[j-1];
	}
	//printf("%u ",d->XL);
	e[firstByteExtent+12]=d->XL;
	//printf("%u ", e[firstByteExtent+12]);
	e[firstByteExtent+13]=d->BC;
	e[firstByteExtent+14]=d->XH;
	e[firstByteExtent+15]=d->RC;
	for (int k=1;k<17;k++)
	{
		e[firstByteExtent+15+k]=d->blocks[k-1];
	}
	blockWrite(e,index);
}


void makeFreeList()
{
	freeList[0]=false;
	for (int boolIndex=1;boolIndex<256;boolIndex++)
	{
		freeList[boolIndex]=true;
	}

	uint8_t *bufferPointer=malloc(sizeof *bufferPointer);
	blockRead(bufferPointer,0);
	for(int i=0;i<32;i++)
	{
			if(bufferPointer[i*32]!=0xe5)
			{
				for(int k=(i*32)+16;k<(i*32)+32;k++)
				{
					if(bufferPointer[k]>=0 && bufferPointer[k]<=255)
					{
						freeList[bufferPointer[k]]=false;
					}
				}
			}
	}
}


void printFreeList()
{
	printf("FREE BLOCK LIST: (* means in-use)\n");
	for(int i=0;i<NUM_BLOCKS;i++)
	{
		if((i%16)==0)
		{
			printf("%3x: ",i);
		}
		if(freeList[i]==false)
		{
			printf("* ");
		}
		if(freeList[i]==true)
		{
			printf(". ");
		}
		if (i % 16 == 15)
		{
			printf("\n");
		}
	}
}


int findExtentWithName(char *name, uint8_t *block0)
{
	if(!checkLegalName(name))
	{
		//printf("Illegal Name");
		return -1;
	}

	else
	{
		int nonEmptyExtents[256];
		//This code identifies all the non-empty extents and put those extent numbers in the array
		int nonFreeExtentsCounter=0;
		for(int i=0;i<32;i++)
		{
			if(block0[i*32]!=0xe5)
			{
				nonEmptyExtents[nonFreeExtentsCounter++]=i;
			}
		}
		//This code splits the given name based on the dot and puts filename and extension name in different arrays
		char *dotPointer=strchr(name,'.');
		char fileString[8];
		char extString[3];
		int spaceIndex=0;
		int matchCounter=0;
		int indexOfDot = (int)(dotPointer-name);//This gives the index of the dot in the given string
		if(dotPointer)
		{
			//printf("Dot is present\n");
			//copying the contents of the 'name' from the beginning till the dot is found into fileString array.
			for(int i=0;i<indexOfDot;i++)
			{
				fileString[i]=name[i];
			}
			//char *extString[(strlen(name)-strlen(fileString))-1];
			int internalK=0;
			//Copying the contents of the 'name' from the dot till the end into extString array.
			for(int extIndex=indexOfDot+1;extIndex<strlen(name);extIndex++)
			{
				extString[internalK++]=name[extIndex];
				//printf("%c ",extString[internalK-1]);
			}
			//printf("Length is %d\n",strlen(extString));
			//Compare fileString with filename in extents

			int extentIndex=0;
			for(extentIndex=0;extentIndex<nonFreeExtentsCounter;extentIndex++)
			{
				//printf("%d\n",nonEmptyExtents[extentIndex]);
				matchCounter=0;
				int k=nonEmptyExtents[extentIndex]*32+1;
				for(int j=0;j<strlen(fileString);j++)
				{
					if(fileString[j]!=block0[k++])
					{
						//printf("Break executed: %c %c %d\n",fileString[j],block0[k-1],nonEmptyExtents[extentIndex]);
						break;
					}
				}
				if(k!=(nonEmptyExtents[extentIndex]*32+1)+strlen(fileString))
				{
					//printf("Continue Executed:%d",nonEmptyExtents[extentIndex]);
					continue;
				}
				else
				{
					//printf("Check for file spaces executed:%d\n",nonEmptyExtents[extentIndex]);
					//Check for file spaces
					if(strlen(fileString)<=7)
					{
						for(spaceIndex=(extentIndex*32)+indexOfDot+1;spaceIndex<(extentIndex*32)+9;spaceIndex++)
						{
							if(block0[spaceIndex]!=0)
							{
								//printf("Break Executed:%d\n",block0[spaceIndex]);
								break;
							}
						}
						if(spaceIndex==(extentIndex*32)+9)
						{
							//printf("%d If Match1\n",nonEmptyExtents[extentIndex]);
							matchCounter++;
						}
						else
						{
							continue;
						}
					}
					else
					{
						//printf("%d Else Match1\n",nonEmptyExtents[extentIndex]);
						matchCounter++;
					}
				}

				//Comparing extensions i.e extString with the extension in name
				int l=nonEmptyExtents[extentIndex]*32+9;
				for(int j=0;j<(strlen(name)-strlen(fileString))-1;j++)
				{
					//printf("%c\n",extString[j]);
					if(extString[j]!=block0[l++])
					{
						//printf("%c %c Extensions Break executed\n",extString[j],block0[l-1]);
						break;
					}
				}
				if(l!=(nonEmptyExtents[extentIndex]*32+9)+(strlen(name)-strlen(fileString))-1)
				{
					//printf("Extensions Continue Executed");
					continue;
				}
				else
				{
					//check for extension spaces
					if((strlen(name)-strlen(fileString))-1<=2)
					{
						//printf("Less than two executed");
						for(spaceIndex=(extentIndex*32)+9+strlen(extString);spaceIndex<(extentIndex*32)+12;spaceIndex++)
						{
							if(block0[spaceIndex]!=0)
							{
								break;
							}
						}
						if(spaceIndex==(extentIndex*32)+12)
						{
							//printf("Match2");
							matchCounter++;
						}
						else
						{
							continue;
						}
					}
					else
					{
						matchCounter++;
					}
				}
				if(matchCounter==2)
				{
					//printf("MArcht");
					return nonEmptyExtents[extentIndex];
				}
			}
			return -1;
		}
		else
		{
			//printf("Dot is not present\n");
			matchCounter=0;
			for(int extentIndexElse=0;extentIndexElse<nonFreeExtentsCounter;extentIndexElse++)
			{
				//printf("%d ",nonEmptyExtents[extentIndexElse]);
				int k=nonEmptyExtents[extentIndexElse]*32+1;
				for(int j=0;j<strlen(name);j++)
				{
					if(block0[k++]!=name[j])
					{
						//printf("Break Executed: %d \n",nonEmptyExtents[extentIndexElse]);
						break;
					}
				}
				if(k!=(nonEmptyExtents[extentIndexElse]*32+1)+strlen(name))
				{
					//printf("Continue Executed: %d\n",k);
					continue;
				}
				else
				{
					if(strlen(name)<=7)
					{
						int spaceIndex=0;
						for(spaceIndex=(nonEmptyExtents[extentIndexElse]*32)+1+strlen(name);spaceIndex<(nonEmptyExtents[extentIndexElse]*32)+9;spaceIndex++)
						{
							if(block0[spaceIndex]!=32)
							{
								//printf("Else NO Dot Strlen: %d \n", nonEmptyExtents[extentIndexElse]);
								break;
							}
						}
						if(spaceIndex==(nonEmptyExtents[extentIndexElse]*32)+9)
						{
							//printf("Space Index: %d\n",nonEmptyExtents[extentIndexElse]);
							matchCounter++;
						}
						else
						{
							continue;
						}
					}
					else
					{
						//printf("Match Counter No Dot");
						matchCounter++;
					}
				}
				//Comparing extent Spaces in buffer with name
				int extentStartsAt=nonEmptyExtents[extentIndexElse]*32+9;
				for(int i=0;i<3;i++)
				{
					if(block0[extentStartsAt++]!=32)
					{
						//printf("Last break");
						break;
					}
				}
				if(extentStartsAt!=nonEmptyExtents[extentIndexElse]*32+12)
				{
					//printf("Last Continue");
					continue;
				}
				else
				{
					matchCounter++;
				}
				if(matchCounter==2)
				{
					return nonEmptyExtents[extentIndexElse];
				}
			}
			return -1;
		}
	}
}

bool checkLegalName(char *name)
{
	//Checks for string length
	if(strlen(name)>=13)
	{
		//printf("Length Executed");
		return false;
	}
	if(strlen(name)<1)
	{
		return false;
	}

	int counter = 0;
	for(int i=0;i<strlen(name);i++)
	{
		if(name[i]=='.')
		{
			counter++;
		}

	}
	//Checks for number of dots allowed in the string.
	if (counter>1)
	{
		//printf("Counter Executed");
		return false;
	}

	//checks if the string has any non-alpha numeric characters.
	for(int j=0;j<strlen(name);j++)
	{
		if(!isalpha(name[j]) && !isdigit(name[j]))
		{
			if(name[j]=='.')
			{
				continue;
			}
			//printf("Alpha");
			return false;
		}
	}

	//Check for the presence of Dot
	int indexOfPointer;
	char *dotPointer=strchr(name,'.');
	if(dotPointer)
	{
		indexOfPointer=(int)(dotPointer-name);
		if(indexOfPointer==0 || indexOfPointer>=9)
		{
			return false;
		}
		else if (&name[strlen(name)-1]-dotPointer>3)
		{
			//printf("Extension greater than 3");
			return false;
		}
	}
	else
	{
		if(strlen(name)>=9)
		{
			return false;
		}
	}
	return true;
}


void cpmDir()
{
	uint8_t blocksCounter=0;
	int sum=0;
	uint8_t *blockReadBuffer=malloc(1024);
	blockRead(blockReadBuffer,0);
	printf("DIRECTORY LISTING\n");
	for(int i=0;i<32;i++)
	{
		sum=0;
		blocksCounter=0;
		if(blockReadBuffer[i*32]!=0xe5)
		{
			for(int fileIndex=(i*32)+1;fileIndex<=(i*32)+8;fileIndex++)
			{
				if(isalpha(blockReadBuffer[fileIndex]) || isdigit(blockReadBuffer[fileIndex]))
				{
					printf("%c",blockReadBuffer[fileIndex]);
				}
			}
			printf("%s",".");
			//Prints Extension of the file
			for(int extIndex=(i*32)+9;extIndex<=(i*32)+11;extIndex++)
			{
				if(isalpha(blockReadBuffer[extIndex]))
				{
					printf("%c",blockReadBuffer[extIndex]);
				}
			}
			sum+=blockReadBuffer[(i*32)+13]+128*(blockReadBuffer[(i*32)+15]);

			for(int sizeIndex=(i*32)+16;sizeIndex<=(i*32)+31;sizeIndex++)
			{
				if(blockReadBuffer[sizeIndex]>0 && blockReadBuffer[sizeIndex]<256)
				{
					blocksCounter++;
				}
			}
			sum+=(blocksCounter-1)*1024;
			printf(" %d",sum);
			printf("\n");
		}
	}
}


int cpmDelete(char * name)
{
	uint8_t *blockReadBuffer=malloc(1024);
	blockRead(blockReadBuffer,0);
	int extentNumber=findExtentWithName(name,blockReadBuffer);
	if(!checkLegalName(name))
	{
		return -2;
	}
	if(extentNumber==-1)
	{
		return -1;
	}
	blockReadBuffer[extentNumber*32]=0xe5;
	for(int i=16;i<32;i++)
	{
		freeList[blockReadBuffer[extentNumber*32+i]]=true;
		blockReadBuffer[extentNumber*32+i]=0;
	}
	freeList[0]=false;
	blockWrite(blockReadBuffer,0);
	return 0;
}

int cpmRename(char *oldName, char * newName)
{
	uint8_t *blockReadBuffer=malloc(1024);
	blockRead(blockReadBuffer,0);
	int extentNumber = findExtentWithName(oldName,blockReadBuffer);
	if(!checkLegalName(oldName) || !checkLegalName(newName))
	{
		return -2;
	}
	else if(extentNumber==-1)
	{
		return -1;
	}

	else if(findExtentWithName(newName,blockReadBuffer)!=-1)
	{
		return -3;
	}
	else
	{
		//Check whether Dot is present or not in the new name
		char *dotPointer=strchr(newName,'.');
		int indexOfDot=(int)(dotPointer-newName);
		if(dotPointer)
		{
			//printf("Dot Present Rename %d\n",indexOfDot);
			int fileStartIndex=extentNumber*32+1;
			int extStartIndex=extentNumber*32+9;
			for(int i=0;i<indexOfDot;i++)
			{
				//printf("first for loop\n");
				blockReadBuffer[fileStartIndex++]=newName[i];
			}
			for(int j=indexOfDot+1;j<strlen(newName);j++)
			{
				blockReadBuffer[extStartIndex++]=newName[j];
			}
			for(int k=fileStartIndex;k<extentNumber*32+9;k++)
			{
				blockReadBuffer[k]=0;
			}
			for(int l=extStartIndex;l<extentNumber*32+12;l++)
			{
				blockReadBuffer[l]=0;
			}
			blockWrite(blockReadBuffer,0);
			//writeImage("image1.img");
			return 0;
		}
		else
		{
			int fileStartIndex=extentNumber*32+1;
			for(int i=0;i<strlen(newName);i++)
			{
				blockReadBuffer[fileStartIndex++]=newName[i];
			}
			blockWrite(blockReadBuffer,0);
			//writeImage("image1.img");
			return 0;
		}
	}
}
