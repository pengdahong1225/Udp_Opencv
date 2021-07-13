﻿#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include<winsock2.h>
#include<stdio.h>
#include<stdlib.h>

#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable : 4996)
using namespace cv;
using namespace std;

/*解码*/
string base64Decode(const char* Data, int DataByte)
{
	const char DecodeTable[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		62, // '+'
		0, 0, 0,
		63, // '/'
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, // '0'-'9'
		0, 0, 0, 0, 0, 0, 0,
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, // 'A'-'Z'
		0, 0, 0, 0, 0, 0,
		26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
		39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, // 'a'-'z'
	};
	//返回值
	std::string strDecode;
	int nValue;
	int i = 0;
	while (i < DataByte)
	{
		if (*Data != '\r' && *Data != '\n')
		{
			nValue = DecodeTable[*Data++] << 18;
			nValue += DecodeTable[*Data++] << 12;
			strDecode += (nValue & 0x00FF0000) >> 16;
			if (*Data != '=')
			{
				nValue += DecodeTable[*Data++] << 6;
				strDecode += (nValue & 0x0000FF00) >> 8;
				if (*Data != '=')
				{
					nValue += DecodeTable[*Data++];
					strDecode += nValue & 0x000000FF;
				}
			}
			i += 4;
		}
		else// 回车换行,跳过
		{
			Data++;
			i++;
		}
	}
	return strDecode;
}
/*编码*/
string base64Encode(const uchar* Data, int DataByte)
{
	const char EncodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	//返回值
	std::string strEncode;
	unsigned char Tmp[4] = { 0 };
	int LineLength = 0;
	for (int i = 0; i < (int)(DataByte / 3); i++)
	{
		Tmp[1] = *Data++;
		Tmp[2] = *Data++;
		Tmp[3] = *Data++;
		strEncode += EncodeTable[Tmp[1] >> 2];
		strEncode += EncodeTable[((Tmp[1] << 4) | (Tmp[2] >> 4)) & 0x3F];
		strEncode += EncodeTable[((Tmp[2] << 2) | (Tmp[3] >> 6)) & 0x3F];
		strEncode += EncodeTable[Tmp[3] & 0x3F];
		if (LineLength += 4, LineLength == 76) { strEncode += "\r\n"; LineLength = 0; }
	}
	//对剩余数据进行编码
	int Mod = DataByte % 3;
	if (Mod == 1)
	{
		Tmp[1] = *Data++;
		strEncode += EncodeTable[(Tmp[1] & 0xFC) >> 2];
		strEncode += EncodeTable[((Tmp[1] & 0x03) << 4)];
		strEncode += "==";
	}
	else if (Mod == 2)
	{
		Tmp[1] = *Data++;
		Tmp[2] = *Data++;
		strEncode += EncodeTable[(Tmp[1] & 0xFC) >> 2];
		strEncode += EncodeTable[((Tmp[1] & 0x03) << 4) | ((Tmp[2] & 0xF0) >> 4)];
		strEncode += EncodeTable[((Tmp[2] & 0x0F) << 2)];
		strEncode += "=";
	}
	cout << "编码成功" << endl;
	return strEncode;
}
string Mat2Base64(const cv::Mat &image,string imgType)
{
	std::string img_data;
	std::vector<uchar> vecImg;
	std::vector<int> vecCompression_params;
	vecCompression_params.push_back(1);
	vecCompression_params.push_back(90);
	imgType = "." + imgType;
	cv::imencode(imgType, image, vecImg, vecCompression_params);
	img_data = base64Encode(vecImg.data(), vecImg.size());
	return img_data;
}

SOCKET Ret_socket()
{
	WSADATA wsadata;
	WORD sockVersion = MAKEWORD(2, 2);
	if (WSAStartup(sockVersion, &wsadata)!=0)
	{
		cout << "WSAStartup初始化失败" << endl;
		return -1;
	}
	SOCKET sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_fd < 0)
	{
		cout << "socket() error" << endl;
		return -1;
	}
	struct sockaddr_in addr_Serv;
	addr_Serv.sin_family = AF_INET;
	addr_Serv.sin_addr.s_addr = inet_addr("192.168.101.9");
	addr_Serv.sin_port = htons(8000);
	uint len = sizeof(addr_Serv);
	bind(sock_fd, (sockaddr*)&addr_Serv, len);
	return sock_fd;
}
/*数据包分片*/
bool udp_piece_cut(string ALL_data, std::vector<string>& pieces)
{
	pieces.push_back(ALL_data.substr(0, 50000));
	pieces.push_back(ALL_data.substr(50000));
	return true;
}

int main()
{
	cv::VideoCapture capture(0);
	Mat frame;
	SOCKET sock_fd = Ret_socket();
	if (sock_fd < 0)
	{
		cout << "socket创建失败" << endl;
		return 0;
	}
	//客户端信息
	sockaddr_in addr_client;
	addr_client.sin_family = AF_INET;
	addr_client.sin_addr.s_addr = inet_addr("192.168.101.9");
	addr_client.sin_port = htons(45454);
	int len2 = sizeof(addr_client);
	while (true)
	{
		capture.read(frame);
		if (frame.empty())
		{
			break;
		}
		cvtColor(frame, frame, COLOR_BGR2GRAY);
		flip(frame, frame, 1);
		string Base64Data = Mat2Base64(frame, "jpg");
		cout << "data size = " << Base64Data.size() << endl;
		bool result = false;
		std::vector<string> pieces;
		/*发送base64字节流,如果大于50000字节则分片发送*/
		if (Base64Data.size() > 50000){
			result = udp_piece_cut(Base64Data, pieces);
		}
		if (result)//分片
		{
			for (uchar i = 0; i < 2; i++)
			{
				const char* sendbuf = pieces[i].c_str();
				if (sendto(sock_fd, sendbuf, strlen(sendbuf), 0, (sockaddr*)&addr_client, len2) != SOCKET_ERROR)
				{
					cout << "分片发送成功" << endl;
				}
			}
		}
		else//不分片
		{
			const char* sendbuf = Base64Data.c_str();
			if (sendto(sock_fd, sendbuf, strlen(sendbuf), 0, (sockaddr*)&addr_client, len2) != SOCKET_ERROR)
			{
				cout << "不分片发发送成功" << endl;
			}
		}
		int c = waitKey(36);
		if (c == 27)
		{
			break;
		}
	}
	closesocket(sock_fd);
	capture.release();
    return 0;
}