/************************************************
 * depend on 
 * ffmpeg
 * 
 * build 
 * g++ mix_test.cpp -o mix_test
 * 
 * run
 * ./mixtest [file1] [file2] [...]
 * for example
 * ./mix_test test1.wav test2.wav 1.wav
************************************************/ 

#include <stdio.h>
#include <stdlib.h> 
#include <string>
#include <list>
#include <iostream>

#define MAX_BLOCK_BUF 10240 * 200
#define MAX_AUDIO_SAMPLE_SIZE 32767
#define MIN_AUDIO_SAMPLE_SIZE -32768
#define OUT_FILE_NAME "outFile"

struct  PCMInfo
{
    int16_t buff[MAX_BLOCK_BUF];
    int sampleNum;
};
typedef std::list<PCMInfo*> PCMList;

int ReadPcm(const char* fileName, int16_t* buff, int* sampleNum);
int mixAudio(PCMList pcmList, int sampleNum, FILE* outFile);
int mixAudio2(PCMList pcmList, int sampleNum, FILE* outFile);

int main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        printf("please enter cmd param, cmd: ./mixtest [file1] [file2] [...]\n");
        return 1;
    }

    // wav to pcm
    for (int i = 1; i < argc; i++)
    {
       char cmd[100] = {0};
       sprintf(cmd, "ffmpeg -i %s -f s16be -ar 44100 -acodec pcm_s16be input%d.raw -y", argv[i], i);
       system(cmd);
    }

    // put pcm data to array
    PCMList pcmList;
    int firtSampleNum = 0;
    char fileName[20] = {0};
    for (int i = 1; i < argc; i++)
    {
        PCMInfo* info = new PCMInfo();
        //std::string fileName = "./output1.raw"; 
        sprintf(fileName, "./input%d.raw", i);
        int ret = ReadPcm(fileName, info->buff, &info->sampleNum);
        if (ret == 1)
        {
            return 1;
        }
        if (i == 1)
        {
            firtSampleNum = info->sampleNum;
        }
        
        printf("read filename:%s\n", fileName);
        pcmList.push_back(info);
    }
	FILE* outFile;
	outFile = fopen("./test_mix_audio.raw", "wb+");
	if (outFile == NULL)
	{
		return 1;
	}

    // mix
    //mixAudio(pcmList, firtSampleNum, outFile);
    mixAudio(pcmList, firtSampleNum, outFile);

    // clear
    fclose(outFile);
    PCMList::const_iterator cit = pcmList.begin();
    for (; cit != pcmList.end(); ++cit)
    {
        delete *cit;
    } 
    // pcm to wav
    system("ffmpeg -f s16be -ar 44100 -ac 2 -acodec pcm_s16be -i test_mix_audio.raw output.wav -y");
    system("rm -f *.raw");
    printf("succss\n");
	return 0;
}

int ReadPcm(const char* fileName, int16_t* buff, int* sampleNum) 
{
	FILE* fp;
	fp = fopen(fileName, "rb");
	if (fp == NULL)
	{
        printf("file:%s, open failed\n",fileName);
		return 1;
	}
    unsigned char *sample=(unsigned char *)malloc(4);
    int num = 0;
    while (!feof(fp))
    {
		fread(sample, 1, 4, fp);
        buff[num++] = *(short*)sample;
        buff[num++] = *(short*)(sample + 2);
        if(num >= MAX_BLOCK_BUF) {
            break;
        }
    }
    *sampleNum  = num;
    printf("file:%s, Sample Cnt:%d\n",fileName, num);
    free(sample);
    fclose(fp);
    return 0;
}

int mixAudio(PCMList pcmList, int sampleNum, FILE* outFile) {
    if (pcmList.size() == 0 || outFile == NULL)
    {
        return 1;
    }

    PCMInfo outPcm;
    PCMList::const_iterator cit = pcmList.begin();
    for (; cit != pcmList.end(); ++cit)
    {
        int index = std::min(sampleNum, (*cit)->sampleNum);
        for (int j = 0; j < index; j++)
        {
            outPcm.buff[j] += (*cit)->buff[j];
            if (outPcm.buff[j] > MAX_AUDIO_SAMPLE_SIZE)
            {
                outPcm.buff[j] = MAX_AUDIO_SAMPLE_SIZE;
            }
            if (outPcm.buff[j] < MIN_AUDIO_SAMPLE_SIZE)
            {
                outPcm.buff[j] = MIN_AUDIO_SAMPLE_SIZE;
            }
        }
        outPcm.sampleNum = index;
    }
    for (int i = 0; i < outPcm.sampleNum; i++)
    {
        fwrite(&outPcm.buff[i], 1, 2, outFile);
    }
    return 0;
}