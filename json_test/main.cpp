#include <stdio.h>
#include <json/json.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <sys/file.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;


Json::Value array;
int count=0;


struct ColorSignature
{
	ColorSignature()
	{
		m_uMin = m_uMax = m_uMean = m_vMin = m_vMax = m_vMean = m_type = 0;
	}	

    int32_t m_uMin;
    int32_t m_uMax;
    int32_t m_uMean;
    int32_t m_vMin;
    int32_t m_vMax;
    int32_t m_vMean;
	uint32_t m_rgb;
	uint32_t m_type;
};
//Json::Value arrayObj;

void  json_write_colorSignature(const struct ColorSignature & sig, int signum)
{
	Json::Value  item;
	Json::Value arrayObj;
	//Json::Value  sig_lable;
	//Json::Value  all;
	//char lable[10] = {0};
	//sprintf(lable,"sig[%d]",signum);
	
	item["lable"] = signum;
	item["m_uMin"] = sig.m_uMin;
	item["m_uMax"] = sig.m_uMax;
	item["m_uMean"] = sig.m_uMean;
	item["m_vMin"] = sig.m_vMin;
	item["m_vMax"] = sig.m_vMax;
	item["m_vMean"] = sig.m_vMean;
	item["m_rgb"] = sig.m_rgb;
	item["m_type"] = sig.m_type;
	arrayObj.append(item);
{
	std::string out = arrayObj.toStyledString();
	std::cout << out << std::endl;
}
	
	//array[(unsigned int)count] = item;
	//array.append(item);
	//count++;
	//sig_lable[lable] = item;
{
	//std::string out = item.toStyledString();
	//std::cout << out << std::endl;
}

	//std::string out = sig_lable.toStyledString();
	//std::cout << out <<std::endl;


	
    //printf("%s,len=%d\r\n",out.c_str(),out.size());

	//std::string save = sig_lable.toStyledString();

	
/*
	long fd = open("save.txt",O_RDWR | O_CREAT  , 0777);
	
	if(fd<=0)
	{
		printf("open file failed\r\n");
	}
	write(fd,save.c_str(),save.size());

	close(fd);
	
	*/

}



int main()
{
#if 0
	int signum = 0;
	//Json::Value  item;
	Json::Value  sig_lable; 

	struct ColorSignature test_json;
	test_json.m_uMax = 1;
	test_json.m_uMin = 2;
	test_json.m_vMax = 3;
	test_json.m_vMin = 4;
	test_json.m_uMean = 5;
	test_json.m_vMean = 6;
	test_json.m_rgb = 7;
	test_json.m_type = 8;

	signum = 1;
	json_write_colorSignature(test_json,signum);

	signum = 2;
	
	json_write_colorSignature(test_json,signum);
#endif

#if 0
	//array[(unsigned int)0] 

	std::string save = array.toStyledString();

	
	long fd = open("save.txt",O_RDWR | O_CREAT  , 0777);
	
	if(fd<=0)
	{
		printf("open file failed\r\n");
	}
	write(fd,save.c_str(),save.size());

	close(fd);
#endif
	int i=0;
	Json::Value  item;
	Json::Value arrayObj;

	struct ColorSignature test_json;
	test_json.m_uMax = 1;
	test_json.m_uMin = 2;
	test_json.m_vMax = 3;
	test_json.m_vMin = 4;
	test_json.m_uMean = 5;
	test_json.m_vMean = 6;
	test_json.m_rgb = 7;
	test_json.m_type = 8;
	for(i=1; i<=7; i++)
	{
		item["lable"] = i;
		item["m_uMin"] = test_json.m_uMin;
		item["m_uMax"] = test_json.m_uMax;
		item["m_uMean"] = test_json.m_uMean;
		item["m_vMin"] = test_json.m_vMin;
		item["m_vMax"] = test_json.m_vMax;
		item["m_vMean"] = test_json.m_vMean;
		item["m_rgb"] = test_json.m_rgb;
		item["m_type"] = test_json.m_type;
		arrayObj.append(item);
	}

//{
	std::string out = arrayObj.toStyledString();
	std::cout << out << std::endl;
//}
	printf("---------------------------\r\n");
//{
		//std::string save = array.toStyledString();
	
		
		long fd = open("save.txt",O_RDWR | O_CREAT	, 0777);
		
		if(fd<=0)
		{
			printf("open file failed\r\n");
		}
		write(fd,out.c_str(),out.size());
	
		close(fd);
//}
	const char *c_data = out.c_str();
	printf("%s\r\n",c_data);
	struct ColorSignature sigall[7];
	Json::Reader reader;
	Json::Value value;
	//std::string strvalue;
	

	printf("--------value---------\r\n");
	reader.parse(c_data,value);

{
	std::string out1 = value.toStyledString();
	std::cout<< out1 << std::endl;
}
	std::cout << value.size() << std::endl;
	//取出value 中的值
	
	//std::cout << value[(unsigned int)0]["lable"].asInt() << std::endl;

	for(i=0; i<value.size(); i++)
	{
		int signum = value[(unsigned int)i]["lable"].asInt();
		sigall[signum-1].m_uMin = value[(unsigned int)i]["m_uMin"].asInt();
		sigall[signum-1].m_uMax = value[(unsigned int)i]["m_uMax"].asInt();
		sigall[signum-1].m_uMean = value[(unsigned int)i]["m_uMean"].asInt();
		sigall[signum-1].m_vMax = value[(unsigned int)i]["m_vMax"].asInt();
		sigall[signum-1].m_vMin = value[(unsigned int)i]["m_vMin"].asInt();
		sigall[signum-1].m_vMean = value[(unsigned int)i]["m_vMean"].asUInt();
		sigall[signum-1].m_rgb = value[(unsigned int)i]["m_rgb"].asUInt();
		sigall[signum-1].m_type = value[(unsigned int)i]["m_type"].asUInt();
	}


	for(i=0; i<7; i++)
	{
		printf("signum:%d,uMin=%d,uMax=%d,uMean=%d,vMax=%d,vMin=%d,vMean=%d,rgb=%d,type=%d\r\n",
			i+1,sigall[i].m_uMin,sigall[i].m_uMax,sigall[i].m_uMean,sigall[i].m_vMax,
			sigall[i].m_vMin,sigall[i].m_vMean,sigall[i].m_rgb,sigall[i].m_type);
	}

	
	return 0;
}


