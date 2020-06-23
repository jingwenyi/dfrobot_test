
#ifndef BASE64_H  
#define BASE64_H 

/* Base64 ���� */   
char* base64_encode(const unsigned char* data, int data_len);   

/* Base64 ���� */   
unsigned char *base64_decode(const char* data, int data_len , int *return_len); 


#endif