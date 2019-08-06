#pragma once
#include<iostream>
#include<sstream>
#include<string>
#include<json/json.h>
#include<memory>
#include<cstdio>
#include<fstream>
#include<map>
#include<unordered_map>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include"speech.h"
#include"base/http.h"
#define SIZE 102

#define ASR_PATH "temp_file/demo.wav"
#define COMMEND_ETC "commend.etc"
#define PLAY_FILE "temp_file/play.wav"
class Util{
    public:
        static bool Exec(std::string command, bool print)
        {
            FILE* fp = popen(command.c_str(), "r");
            if(NULL == fp){
                std::cerr << "popen error!" << std::endl;
                return false;
            }
            if(print){
                char c;
                while(fread(&c, 1, 1, fp) >0 )
                {
                    std::cout << c;
                }
                std::cout << std::endl;
            }
            pclose(fp);
            return true;
        }
        static void* Clock(void* arg)
	    {
            pthread_detach(pthread_self());
	        char* bar = (char*)malloc(SIZE * sizeof(char));
	        memset(bar, '\0', SIZE*sizeof(char));
	        const char* lable = "|/-\\";
	        int i = 0;
	        for(; i < SIZE-1; ++i)
	        {
	          //printf("[%-100s][%d%%][%c]\r", bar, i, lable[i%4]);
	          //printf("[请讲话][%d%%][%c][%-100s]\r",i, lable[i%4],bar);
	          printf("[请讲话][%-100s][%d%%]\r",bar,i);
	          fflush(stdout);
	          bar[i] = '>';
	          usleep(60000);
	        }
            std::cout << std::endl;
            return (void*)0;
	    }
};

class Robot{
    private:
        std::string url;
        std::string api_key;
        std::string user_id;
        aip::HttpClient client;
    private:
        std::string ConversionToJson(std::string& message)
        {
            Json::Value root;
            Json::Value item1;
            Json::Value item2;
            Json::StreamWriterBuilder swb;
            std::ostringstream os;

            root["reqType"] = 0;
            item1["text"] = message;
            item2["inputText"] = item1;
            item1.clear();
            root["perception"] = item2;
            item2.clear();
            item2["apiKey"] = api_key;
            item2["userId"] = user_id;
            root["userInfo"] = item2;
            item2.clear();

            std::unique_ptr<Json::StreamWriter> wb(swb.newStreamWriter());
            wb->write(root,&os);
            std::string ret = os.str();
            return ret;
        }
        std::string PostRequest(std::string& message)
        {
            //std::cout << message << std::endl;
            std::string response;
            int code = this->client.post(this->url, nullptr, message, nullptr, &response);
		    if(code != CURLcode::CURLE_OK){
                std::cout << "post error!" << std::endl;
                exit(2);
            }
            return response;
        }
        std::string ConversionToString(std::string& massage)
        {
            JSONCPP_STRING errs;
            Json::Value root;
            Json::CharReaderBuilder rb;

            std::unique_ptr<Json::CharReader> const cr(rb.newCharReader());
            bool res = cr->parse(massage.data(), massage.data()+massage.size(), &root, &errs);
            if(!res || !errs.empty()){
                std::cerr <<"JsonToString error!" << std::endl;
                exit(3);
            }
            std::string ret;
            ret = root["results"][0]["values"]["text"].asString();
            return ret;
        }
    public:
        Robot(std::string id = "1"):user_id(id)
        {
            url = "http://openapi.tuling123.com/openapi/api/v2";
		    api_key = "208530129de04d6d82bea28a0404c582";
        }
        std::string Talk(std::string& message)
        {
            std::string str = ConversionToJson(message);
            std::string str1 = PostRequest(str);
            std::string str2 = ConversionToString(str1);
            //std::cout << "Friday Reply# " << str2 << std::endl;
            return str2;
        }
        ~Robot()
        {

        }
};

class SpeechRec{
    private:
        std::string app_id = "16869259";
        std::string api_key = "mBbgjcXD37i9gPGhOFM7d3c2";
        std::string secret_key = "yo2qmVX2pUKGQO3Z7IhWf328jFEoFFdw";
        aip::Speech* client;
    public:
        SpeechRec()
        {
            client = new aip::Speech(app_id, api_key, secret_key);
        }
        void ASR(std::string& message, int& code)
        {
            std::map<std::string, std::string> options;
            options["dev_pid"] = "1536";
            std::string file_content;
            aip::get_file_content(ASR_PATH, &file_content);
            Json::Value result = client->recognize(file_content, "wav", 16000, options);
            //std::cout << result.toStyledString() << std::endl;
            code = result["err_no"].asInt();
            if(code == 0){
                message = result["result"][0].asString();
            }
        }
	    void TTS(std::string message)
	    {
	    	//std::cout << "TTS : " << message << std::endl;
	    	std::ofstream ofile;
	    	std::string file_ret;
	    	std::map<std::string, std::string> options;
	    	options["spd"] = "5";
	    	options["per"] = "4";
            options["aue"] = "6";
	    	ofile.open(PLAY_FILE, std::ios::out | std::ios::binary);
	    	//语音合成，将文本转成语音，放到指定目录，形成指定文件
	    	Json::Value result = client->text2audio(message, options, file_ret);
	    	if(!file_ret.empty()){
	    	    ofile << file_ret;
	    	}
	    	else{
	    	    std::cout << "error: " << result.toStyledString();
	    	}
            ofile.close();
	    }
};

class Friday{
    private:
        Robot rb;
        SpeechRec sr;
        std::unordered_map<std::string, std::string> commends;
    private:
        bool RecordandAsr(std::string& message)
        {
            int err_code = -1;
            std::string record = "arecord -t wav -c 1 -r 16000 -d 6 -f S16_LE ";
            record += ASR_PATH;
            record += " >/dev/null 2>&1";

            pthread_t tid;
            pthread_create(&tid, NULL, Util::Clock, NULL);
            if(Util::Exec(record, false)){
                sr.ASR(message, err_code);
                if(err_code == 0){
                    return true;
                }
                std::cout << "语音识别失败..." << std::endl;
            }
            else{
                std::cout << "语音录制失败..." << std::endl;
            }
            return false;
        }
        bool IsCommend(std::string& massage)
        {
            return commends[massage] == "" ? false : true;
        }
    public:
	    bool TTSAndPlay(std::string message)
	    {
	    	//cvlc命令行式的播放
	    	std::string play = "cvlc --play-and-exit ";//播放完毕退出：--play-and-exit
	    	play += PLAY_FILE;
	    	play += " >/dev/null 2>&1";
	    	sr.TTS(message); //语音识别
            Util::Exec(play, false); //执行播放
	    	return true;
	    }
        void LoadCommend()
        {
            char buf[256];
            std::ifstream in(COMMEND_ETC);
            if(!in.is_open()){
                std::cerr << "open error" << std::endl;
                exit(1);
            }
            std::string sep = ":";
            while(in.getline(buf,sizeof(buf))){
                std::string str = buf;
                size_t pos = str.find(sep);
                if(pos == std::string::npos){
                    std::cout << "加载文件失败:" << str <<std::endl;
                    exit(2);
                }
                std::string k = str.substr(0,pos);
                std::string v = str.substr(pos+sep.size());
                commends[k] = v;
            }
            std::cout << "加载文件完毕!" << std::endl;
            in.close();
        }
        void Run()
        {
            volatile bool quit = false;
            std::string message;
            while(!quit){
                if(!this->RecordandAsr(message)){
                    continue;
                }
                //if(message == "退出。"){
                //    std::cout << "再见啦！！" <<std::endl;
                //    quit = true; continue;
                //}
                if(IsCommend(message)){
                    Util::Exec(commends[message], true);
                }
                else{
                    std::cout << "MyTalk#" << message <<std::endl;
                    std::string ret = rb.Talk(message);
                    std::cout << "Friday# " << ret << std::endl;
                    TTSAndPlay(ret);
                }
            }
        }
};
