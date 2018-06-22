/* ----------------------------------------------------------------- */
/*           The HMM-Based Singing Voice Synthesis System "Sinsy"    */
/*           developed by Sinsy Working Group                        */
/*           http://sinsy.sourceforge.net/                           */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2009-2015  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the Sinsy working group nor the names of    */
/*   its contributors may be used to endorse or promote products     */
/*   derived from this software without specific prior written       */
/*   permission.                                                     */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "sinsy.h"

#include <espeak-ng/espeak_ng.h>
#include <espeak-ng/speak_lib.h>
#include <espeak-ng/encoding.h>
#include <sndfile.h>
#include <samplerate.h>
#include <assert.h>
#include <ringbuffer.h>
#include <unistd.h>

using namespace std;

namespace
{
//const char* DEFAULT_LANGS = "fi";
};

void usage()
{
#ifdef HAVE_HTS
   std::cout << "The HMM-Based Singing Voice Syntheis System \"Sinsy\"" << std::endl;
#else
   std::cout << "The Formant-Based Singing Voice Syntheis System \"SinsyNG\"" << std::endl;
#endif
   std::cout << "Version 0.92 (http://sinsy.sourceforge.net/)" << std::endl;
   std::cout << "Copyright (C) 2009-2015 Nagoya Institute of Technology" << std::endl;
   std::cout << "All rights reserved." << std::endl;
   std::cout << "" << std::endl;
   #ifdef HAVE_HTS
   std::cout << "The HMM-Based Speech Synthesis Engine \"hts_engine API\"" << std::endl;
   std::cout << "Version 1.10 (http://hts-engine.sourceforge.net/)" << std::endl;
   std::cout << "Copyright (C) 2001-2015 Nagoya Institute of Technology" << std::endl;
   std::cout << "              2001-2008 Tokyo Institute of Technology" << std::endl;
   std::cout << "All rights reserved." << std::endl;
   std::cout << "" << std::endl;
   std::cout << "sinsy - The HMM-based singing voice synthesis system \"Sinsy\"" << std::endl;
   #else
   std::cout << "The eSpeak NG (Next Generation) Text-to-Speech Synthesis Engine" << std::endl;
   std::cout << "Copyright (C) 2005-2014 Jonathan Duddington" << std::endl;
   std::cout << "              2015-2017 Reece H. Dunn" << std::endl;
   #endif
   std::cout << "" << std::endl;
   std::cout << "  usage:" << std::endl;
   std::cout << "    sinsy [ options ] [ infile ]" << std::endl;
   std::cout << "  options:                                           [def]" << std::endl;
   
   #ifdef HAVE_HTS
   std::cout << "    -w langs    : languages                          [  j]" << std::endl;
   std::cout << "                  j: Japanese                             " << std::endl;
   std::cout << "                  (Currently, you can set only Japanese)  " << std::endl;
   #else
   std::cout << "    -w langs    : languages                          [ en]" << std::endl;
   #endif
   
   #ifdef HAVE_HTS
   std::cout << "    -x dir      : dictionary directory               [N/A]" << std::endl;
   std::cout << "    -m htsvoice : HTS voice file                     [N/A]" << std::endl;
   #endif
   std::cout << "    -o file     : filename of output wav audio       [N/A]" << std::endl;
   std::cout << "  infile:" << std::endl;
   std::cout << "    MusicXML file" << std::endl;
}

static int SynthCallback(short *wav, int numsamples, espeak_EVENT *events)
{
	//dummy
	return 0;
}

class ECantorix : public sinsy::IScore
{
	int inputSamplerate=0;
	int outputSamplerate=0;
    bool inTie=false;
    size_t tieDuration=0;
    std::string tieLyrics;
    
    float* resample(float* data_in,int input_frames,float ratio,int *output_frames)
	{
		*output_frames = input_frames*ratio;
		float* ret = (float*)malloc(sizeof(float)*output_frames[0]);
		SRC_DATA data;
		data.data_in = data_in;
		data.data_out = ret;
		data.input_frames = input_frames;
		data.output_frames = *output_frames;
		data.src_ratio = ratio;
		src_simple(&data,SRC_SINC_BEST_QUALITY,1);
		return ret;
	}

    
public:
    ECantorix(){
    }
    virtual ~ECantorix() {
    }
    
    void init()
    {
        char *data_path = NULL; // use default path for espeak-ng-data
        espeak_ng_InitializePath(data_path);
        espeak_ng_ERROR_CONTEXT context = NULL;
        espeak_ng_STATUS result = espeak_ng_Initialize(&context);
        if (result != ENS_OK) {
            espeak_ng_PrintStatusCodeMessage(result, stderr, context);
            espeak_ng_ClearErrorContext(&context);
            exit(1);
        }
        sinsy_ng_Init();
        result = espeak_ng_InitializeOutput(ENOUTPUT_MODE_SYNCHRONOUS, 0, NULL);
		espeak_SetSynthCallback(SynthCallback);//dummy synth callback
        inputSamplerate = espeak_ng_GetSampleRate();
    }
    
    void setOutputSamplerate(int fs)
    {
      outputSamplerate = fs;
    }
    
    
    void setVoiceByName(const std::string& voicename)
    {
        espeak_ng_STATUS result = espeak_ng_SetVoiceByName(voicename.c_str());
        if (result != ENS_OK) {
             fprintf(stderr,"ESPEAK_ERROR voice %s not found\n",voicename.c_str());
             exit(1);
        }
    }

    //! set encoding
    virtual bool setEncoding(const std::string& encoding)
    {
        if(encoding=="utf-8") return true;
        fprintf(stderr,"setEncoding %s\n",encoding.c_str());
        return false;
    }

    //! add key mark
    virtual bool addKeyMark(sinsy::ModeType modeType, int fifths)
    {
        fprintf(stderr,"addKeyMark %i %i\n",(int)modeType,fifths);
        return true;
    }

    //! add beat mark (beats/beatType) to end of score: default beat mark is 4/4
    virtual bool addBeatMark(size_t beats, size_t beatType)
    {
        fprintf(stderr,"addBeatMark %i %i\n",(int)beats,(int)beatType);
        return true;
    }

    //! add tempo mark to end of score: default tempo is 100bps
    virtual bool addTempoMark(double tempo)
    {
        fprintf(stderr,"addTempoMark %f\n",tempo);
        return true;
    }

    //! add dynamics mark (sudden changes) to end of score
    virtual bool addSuddenDynamicsMark(sinsy::SuddenDynamicsType suddenDynamicsType)
    {
        fprintf(stderr,"addSuddenDynamicsMark %i\n",(int)suddenDynamicsType);
        return true;
    }

    //! add dynamics mark (gradual changes) to end of score
    virtual bool addGradualDynamicsMark(sinsy::GradualDynamicsType gradualDynamicsType)
    {
        fprintf(stderr,"addGradualDynamicsMark %i\n",(int)gradualDynamicsType);
        return true;
    }

    //! add note to end of score
    virtual bool addNote(size_t duration, const std::string& lyric, size_t pitch, bool accent, bool staccato, sinsy::TieType tieType, sinsy::SlurType slurType, sinsy::SyllabicType syllabicType, bool breath = false)
    {
        if(tieType==sinsy::TIETYPE_BEGIN) inTie=true;
        if(inTie) {
            tieLyrics+=lyric;
            tieDuration+=duration;
        }
        if(!inTie) sinsy_ng_addNote(duration,lyric.c_str(),pitch,accent,staccato,slurType,syllabicType,breath);
        if(tieType==sinsy::TIETYPE_END)
        {
            sinsy_ng_addNote(tieDuration,tieLyrics.c_str(),pitch,accent,staccato,slurType,syllabicType,breath);
            tieDuration=0;
            tieLyrics="";
            inTie=false;
        }
        //fprintf(stderr,"addNote %i [%s] pitch=%i accent=%i staccato=%i tieType=%i slurType=%i syllabicType=%i breath=%i\n",duration,lyric.c_str(),pitch,accent,staccato,tieType,slurType,syllabicType,breath);
        return true;
    }

    //! add rest to end of score
    virtual bool addRest(size_t duration)
    {
        if(inTie)
        {
            fprintf(stderr,"cannot tie rests\n");
            exit(1);
        }
        sinsy_ng_addRest(duration);
       // fprintf(stderr,"addRest %i\n",duration);
        return true;
    }
    
    void saveTo(std::string fileName)
    {
		
		int data_length = 0;
		float* data2=0;
		float* data = sinsy_ng_getAudioData(&data_length);
		int fs=inputSamplerate;
		if(outputSamplerate!=0 && outputSamplerate!=inputSamplerate)
		{
			fs=outputSamplerate;
			int data_length2;
			data2 = data;
			float ratio = outputSamplerate*1.0/inputSamplerate;
			data = resample(data,data_length,ratio,&data_length2);
			fprintf(stderr,"resample %i %i\n",data_length,data_length2);
			data_length = data_length2;
		}
		
		
		
		SF_INFO info;
		memset(&info,0,sizeof(info));
		info.channels = 1;
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        info.samplerate = fs;
        SNDFILE* sndfile = sf_open(fileName.c_str(),SFM_WRITE,&info);
        sf_write_float(sndfile,data,data_length);
        free(data);
        sf_close(sndfile);
        fprintf(stderr,"saving to %s length=%i\n",fileName.c_str(),data_length);
        if(data2) free(data2);
	}
};

ECantorix ecantorix;

class CommandHandler
{
    vector<string> argv;
void rest()
{
    if(argv.size()>1) ecantorix.addRest(atoi(argv[1].c_str()));
}
void note()
{
    bool breath = false;
    if(argv.size()>9) breath = atoi(argv[9].c_str());
    if(argv.size()>8) ecantorix.addNote(atoi(argv[1].c_str()),argv[2],atoi(argv[3].c_str()),
                (bool)atoi(argv[4].c_str()),(bool)atoi(argv[5].c_str()),
                (sinsy::TieType)atoi(argv[6].c_str()),(sinsy::SlurType)atoi(argv[7].c_str()),(sinsy::SyllabicType)atoi(argv[8].c_str()),breath);
}
void resample()
{
    if(argv.size()>1) ecantorix.setOutputSamplerate(atoi(argv[1].c_str()));
}
void voice()
{
    if(argv.size()>1) ecantorix.setVoiceByName(argv[1]);
}
public:
void parseCMD(const string& input)
{
    
	std::string tmp;
	bool quote = false;
	for(int i=0;i<input.length();i++)
	{
        if(i==0 && input[i]=='#') return;//comment
		if(input[i]=='\"' && quote==false)
		{
			quote=true;
		}
		else if(input[i]=='\"' && quote==true)
		{
			quote=false;
		}
		else if(input[i]==' ' && quote==false)
		{
            if(tmp.length()) argv.push_back(tmp);
			tmp = "";
		}
		else if(i<input.length()-1 && input[i]=='\\')
		{
			i++;
			tmp += input[i];
		}
		else
		{
			tmp += input[i];
		}
	}
	if(tmp.length()) argv.push_back(tmp);
    string cmd = argv[0];
    if(cmd=="note") note();
    if(cmd=="rest") rest();
    if(cmd=="resample") resample();
    if(cmd=="voice") voice();
    argv.clear();
}
};


int handleUScore(const std::string& uscore,const std::string& wav) {
    ecantorix.init();
    std::ifstream input(uscore);
    CommandHandler cmd;
    while(input) {
	string input_line;
    getline(input, input_line);
	cmd.parseCMD(input_line);
    };
    ecantorix.saveTo(wav);
    return 0;
}

int main(int argc, char **argv)
{
   if (argc < 2) {
      usage();
      return -1;
   }

   std::string xml;
   std::string voice;
   #ifdef HAVE_HTS
   std::string config;
   #endif
   std::string wav;
   std::string languages;
   std::string uscore;
   
   voice = "en";
   
   int i(1);
   for(; i < argc; ++i) {
      if ('-' != argv[i][0]) {
         if (xml.empty()) {
            xml = argv[i];
         } else {
            std::cout << "[ERROR] invalid option : '" << argv[i][1] << "'" << std::endl;
            usage();
            return -1;
         }
      } else {
         switch (argv[i][1]) {
         case 'w' :
            languages = argv[++i];
            break;
         #ifdef HAVE_HTS
         case 'x' :
            config = argv[++i];
            break;
         #endif   
         case 'm' :
            voice = argv[++i];
            break;
         case 'o' :
            wav = argv[++i];
            break;
         case 'u' :
            uscore = argv[++i];
            break;
         case 'h' :
            usage();
            return 0;
         default :
            std::cout << "[ERROR] invalid option : '-" << argv[i][1] << "'" << std::endl;
            usage();
            return -1;
         }
      }
   }
   
   if(uscore.size()) {  
       return handleUScore(uscore,wav);
   }

   if(xml.empty() || voice.empty() || wav.empty()) {
	   
      usage();
      return -1;
   }
   
   

   sinsy::Sinsy sinsy;

   std::vector<std::string> voices;
   voices.push_back(voice);

   #ifdef HAVE_HTS
   if (!sinsy.setLanguages(languages, config)) {
      std::cout << "[ERROR] failed to set languages : " << languages << ", config dir : " << config << std::endl;
      return -1;
   }

   if (!sinsy.loadVoices(voices)) {
      std::cout << "[ERROR] failed to load voices : " << voice << std::endl;
      return -1;
   }
   #endif

   if (!sinsy.loadScoreFromMusicXML(xml)) {
      std::cout << "[ERROR] failed to load score from MusicXML file : " << xml << std::endl;
      return -1;
   }
   
   #ifdef HAVE_HTS

   sinsy::SynthCondition condition;

   if (wav.empty()) {
      condition.setPlayFlag();
   } else {
      condition.setSaveFilePath(wav);
   }

   sinsy.synthesize(condition);
   
   
   #else
  
   
   
   ECantorix ecantorix;
   ecantorix.init();
   ecantorix.setVoiceByName(voice);
   sinsy.toScore(ecantorix);
   
   
   #if 1
   if (wav.empty()) {
	  // use https://github.com/espeak-ng/pcaudiolib
      //ecantorix.play();//
   } else {
      ecantorix.saveTo(wav);
   }
   #endif
   
   
   
   
   
   
  // if(g_sndfile) sf_close(g_sndfile);
	
   
   #endif

   return 0;
}
