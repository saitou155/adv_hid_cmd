#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <hidapi/hidapi.h>

#define VENDOR_ID  0x22ea
#define PRODUCT_ID 0x003a

#define BUFF_SIZE 64
#define IR_FREQ_DEFAULT	38000
static unsigned short frequency = IR_FREQ_DEFAULT;

int varbose = 0;
int trans_count = 0;
unsigned char trans_data[9600+1];	
void dump(char cType, const unsigned char* data,int size);

hid_device* adv_open(int nDevNo)
{
	hid_device* device = NULL;
	int i = -1;

	struct hid_device_info *poi,*info = NULL;
	info = hid_enumerate(VENDOR_ID, PRODUCT_ID);
	if(info == NULL)
	{
		goto EXIT_PATH;
	}
	poi = info;
	while(poi)
	{
		if(poi->interface_number == 3)
		{
			i += 1;
			if(i == nDevNo)
			{
				break;
			}
		}
		poi = poi->next;
	}
	if(poi == NULL)
	{
		goto EXIT_PATH;
	}
	device = hid_open_path(poi->path);
EXIT_PATH:
	if(info != NULL)
	{
		hid_free_enumeration(info);
	}
	return device;
}

int avd_write(hid_device* device, const unsigned char* data,size_t length)
{
	int ret;
	ret = hid_write(device, data, length);
	if(varbose)
	{
		if(ret >= 0)
		{
			dump('S', data, ret);
		}
	}
	return ret;
}

int avd_read(hid_device* device, unsigned char* data,size_t length,int mS)
{
	int ret;
	ret = hid_read_timeout(device, data, length, mS);
	if(varbose)
	{
		if(ret >= 0)
		{
			dump('R', data, ret);
		}
	}
	return ret;
}

int adv_close(hid_device* device) 
{
    if(!device)
    {
        return -1;
    }
	hid_close(device);
    return 0;
}

int adv_version(hid_device* device)
{
	int ret;
	unsigned char buf[BUFF_SIZE+1];

	memset(buf,0xff,sizeof(buf));
	buf[0] = 0x00;
	buf[1] = 0x56;
	ret = avd_write(device, buf, sizeof(buf));
	if(ret < 0)
	{
		fprintf(stderr, "ERROR:Can't write get version.\n");
		return -1;
	}
	memset(buf,0xff,BUFF_SIZE);
	ret = avd_read(device, buf, BUFF_SIZE, 500);
	if(ret < 0)
	{
		fprintf(stderr, "ERROR:Can't read get version.\n");
		return -1;
	}
	fprintf(stderr, "Firmware version %5s.\n",&buf[1]);
	return 0;
}

int adv_recv_start(hid_device* device)
{
	int ret;
	unsigned char buf[BUFF_SIZE + 1];

	memset(buf,0xff,sizeof(buf));
	buf[0] = 0x00;
	buf[1] = 0x31;
    buf[2] = (unsigned char)((frequency >> 8) & 0xFF);
    buf[3] = (unsigned char)((frequency >> 0) & 0xFF);
    buf[4] = 0;
    buf[5] = 0;     // 読み込み停止ON時間MSB
    buf[6] = 0;     // 読み込み停止ON時間LSB
    buf[7] = 0;     // 読み込み停止OFF時間MSB
    buf[8] = 0;     // 読み込み停止OFF時間LSB
	ret = avd_write(device, buf, sizeof(buf));
	if(ret < 0)
	{
		fprintf(stderr, "ERROR:Can't write receord start. \n");
		return -1;
	}
	memset(buf,0xff,BUFF_SIZE);
	ret = avd_read(device, buf, BUFF_SIZE, 500);
	if(ret < 0)
	{
		fprintf(stderr, "ERROR:Can't read receord start. \n");
		return -1;
	}
	if((buf[0] != 0x31) || (buf[1] != 0x00))
	{
		fprintf(stderr, "ERROR:failed to receord start.\n");
		return -1;
	}
	return 0;
}

int adv_recv_stop(hid_device* device)
{
	int ret;
	unsigned char buf[BUFF_SIZE + 1];

	memset(buf,0xff,sizeof(buf));
	buf[0] = 0x00;
	buf[1] = 0x32;
	ret = avd_write(device, buf, sizeof(buf));
	if(ret < 0)
	{
		fprintf(stderr, "ERROR:Can't write receord stop.\n");
		return -1;
	}
	memset(buf,0xff,BUFF_SIZE);
	ret = avd_read(device, buf, BUFF_SIZE, 500);
	if(ret < 0)
	{
		fprintf(stderr, "ERROR:Can't read receord stop.\n");
		return -1;
	}
	if(buf[0] == 0x32) 
	{
		return 0;
	}
	fprintf(stderr, "ERROR:failed to receord stop.\n");
	return -1;
}

int adv_recv_read(hid_device* device)
{
	int ret;
	unsigned char buf[BUFF_SIZE + 1];
	int all,off,sz;

	for(int i = 0;;i++)
	{
		memset(buf,0xff,sizeof(buf));
		buf[0] = 0x00;
		buf[1] = 0x33;
		ret = avd_write(device, buf, sizeof(buf));
		if(ret < 0)
		{
			fprintf(stderr, "ERROR:Can't write receord read.\n");
			return -1;
		}
		memset(buf,0xff,BUFF_SIZE);
		ret = avd_read(device, buf, BUFF_SIZE, 500);
		if(ret < 0)
		{
			fprintf(stderr, "ERROR:Can't read receord read.\n");
			return -1;
		}
		if(buf[0] != 0x33) 
		{
			fprintf(stderr, "ERROR:failed to receord read.\n");
			return -1;
		}
		all = ((((int)buf[1]) & 0xff) << 8) + (((int)buf[2]) & 0xff);
		off = ((((int)buf[3]) & 0xff) << 8) + (((int)buf[4]) & 0xff);
		sz  = (((int)buf[5]) & 0xff);

		if(all == 0)
		{
			fprintf(stderr,"Timeout.\n");
			break;
		}

		if(all == off)
		{
			printf("\n");
			break;
		}
		if(sz != 0)
		{
			for(int j = 0;j < sz;j++)
			{
				for(int k = 0;k < 4;k++)
				{
					printf("0x%02x",((int)buf[6 + (j * 4) + k]) & 0xff);
					if(((off + sz) == all) && (j == (sz - 1)) && (k == 3))
					{
						break;
					}
					printf(",");
				}
			}
		}
	}
	fflush(stderr);
	return 0;
}

int adv_receive()
{
	hid_device* device = NULL;
	int res = -1;
	int ret;
	device = adv_open(0);
	if(!device)
	{
		fprintf(stderr,"Can't open device.\n");
		goto EXIT_PATH;
	}

	if(varbose)
	{
		ret = adv_version(device);
		if(ret)
		{
			goto EXIT_PATH;
		}
	}

	if(varbose)
	{
		fprintf(stderr,"receord start.\n");
	}
	ret = adv_recv_start(device);
	if(ret)
	{
		goto EXIT_PATH;
	}
	sleep(3);
	if(varbose)
	{
		fprintf(stderr,"receord stop.\n");
	}
	ret = adv_recv_stop(device);
	if(ret)
	{
		goto EXIT_PATH;
	}
	usleep(100 * 1000);
	if(varbose)
	{
		fprintf(stderr,"receord read.\n");
	}
	ret = adv_recv_read(device);
	if(ret)
	{
		goto EXIT_PATH;
	}
	res = 0;
EXIT_PATH:
	if(device != NULL) 
	{
		ret = adv_close(device);
	}
	return res;
}

#define PACKET_MAX_NUM ((BUFF_SIZE - 5) / 4)
int adv_transfer(unsigned char sdata[],int size)
{
	hid_device* device = NULL;
	int res = -1;
	int ret;
	unsigned char buf[BUFF_SIZE+1];
	int all,sz;

	device = adv_open(0);
	if(!device)
	{
		fprintf(stderr,"Can't open device.\n");
		goto EXIT_PATH;
	}

	if(varbose)
	{
		ret = adv_version(device);
		if(ret)
		{
			goto EXIT_PATH;
		}
	}
	
	all = (size / 4) + ((size % 4) ? 1 : 0);
	for(int i = 0;i < all;i += PACKET_MAX_NUM)
	{
		if(varbose)
		{
			fprintf(stderr,"transfer data set.\n");
		}

		sz = ((i + PACKET_MAX_NUM) <= all) ? PACKET_MAX_NUM : (all - i);
		memset(buf,0xff,sizeof(buf));
		buf[0] = 0x00;
		buf[1] = 0x34;
		buf[2] = (unsigned char)((all >> 8) & 0xff);
		buf[3] = (unsigned char)((all >> 0) & 0xff);
		buf[4] = (unsigned char)((i >> 8) & 0xff);
		buf[5] = (unsigned char)((i >> 0) & 0xff);
		buf[6] = (unsigned char)sz;

		for(int j = 0;j < sz;j++)
		{
			for(int k = 0;k < 5;k++)
			{
				buf[7 + (j * 4) + k] = sdata[((i + j) * 4) + k];
			}
		}
		ret = avd_write(device, buf, sizeof(buf));
		if(ret < 0)
		{
			fprintf(stderr, "ERROR:Can't write transfer data set.\n");
			return -1;
		}
		memset(buf,0xff,BUFF_SIZE);
		ret = avd_read(device, buf, BUFF_SIZE, 500);
		if(ret < 0)
		{
			fprintf(stderr, "ERROR:Can't read transfer data set.\n");
			return -1;
		}
		if((buf[0] != 0x34) || (buf[1] != 0x00)) 
		{
			fprintf(stderr, "ERROR:failed to transfer data set.\n");
			goto EXIT_PATH;
		}
	}

	if(varbose)
	{
		fprintf(stderr,"transfer execution.\n");
	}
	memset(buf,0xff,sizeof(buf));
	buf[0] = 0x00;
	buf[1] = 0x35;
	buf[2] = (unsigned char)((frequency >> 8) & 0xff);
	buf[3] = (unsigned char)((frequency >> 0) & 0xff);
	buf[4] = (unsigned char)((all >> 8) & 0xff);
	buf[5] = (unsigned char)((all >> 0) & 0xff);
	ret = avd_write(device, buf, sizeof(buf));
	if(ret < 0)
	{
		fprintf(stderr, "ERROR:Can't write transfer execution.\n");
		return -1;
	}
	memset(buf,0xff,BUFF_SIZE);
	ret = avd_read(device, buf, BUFF_SIZE, 500);
	if(ret < 0)
	{
		fprintf(stderr, "ERROR:Can't read transfer execution.\n");
		return -1;
	}
	if((buf[0] != 0x35) || (buf[1] != 0x00)) 
	{
		fprintf(stderr, "ERROR:failed to transfer execution.\n");
		goto EXIT_PATH;
	}
	res = 0;
EXIT_PATH:
	if(device != NULL) 
	{
		ret = adv_close(device);
	}
	return res;
}

int hex2ary(const char* hex)
{
	int ret,len;
	unsigned int x;
	char c;
	char tmp[16+1];
	
	trans_count = 0;
	memset(trans_data,0,sizeof(trans_data));
	
	memset(tmp,0,sizeof(tmp));
	len = strlen(hex);
	for(int i = 0;i < (len + 1);i++)
	{
		if((*hex == ',') || 
		   i == len)
		{
			if(strlen(tmp))
			{
				ret = sscanf(tmp,"%x%c",&x,&c);
				if(ret == 1)
				{
					trans_data[trans_count] = (unsigned char)x;
					trans_count++;
					if(trans_count >= sizeof(trans_data))
					{
						fprintf(stderr,"ERROR:Hex string is too long.\n");
						return -1;
					}
					memset(tmp,0,sizeof(tmp));
				}
				else
				{
					fprintf(stderr,"ERROR:Incorrect hex number. %s\n",tmp);
					return -1;
				}
			}
		}
		else
		{
			int len = strlen(tmp);
			tmp[len] = *hex;
			if(len >= 16)
			{
				fprintf(stderr,"ERROR:Hex digit is too long.\n");
				return -1;
			}
		}
		hex++;
	}
	fprintf(stderr,"%d bytes\n",trans_count);
	return 0;
}

void usage() 
{
  fprintf(stderr, "usage: adv_hid_cmd <option>\n");
  fprintf(stderr, "  -r       \tReceive Infrared code.\n");
  fprintf(stderr, "  -t 'hex'\tTransfer Infrared code.\n");
  fprintf(stderr, "  -t \"$(cat XXX.txt)\"\n");
  fprintf(stderr, "  -t \"`cat XXX.txt`\"\n");
  fprintf(stderr, "  -v       \t varbose mode.\n");
}

int main(int argc,char* argv[])
{
	int ret = -1;
	char cmd = '\0';
	char* hex = NULL;
	
	int opt;
	while ((opt = getopt(argc, argv, "vrt:")) != -1) 
	{
        switch (opt) 
		{
            case 'r':
				cmd = opt;
                break;
            case 'v':
				varbose = 1;
                break;
            case 't':
				cmd = opt;
				hex = optarg;
                break;
        }
    }

	if(cmd == '\0')
	{
		usage();
		return 0;
	}
	
	/* hidapi initialize*/
	if ((ret = hid_init()) < 0) {
		perror("hid_init\n");
		return -1;
	}

	switch(cmd)
	{
		case 'r': 
			ret = adv_receive();
			break;
		case 't': 
			ret = hex2ary(hex);
			if(ret)
			{
				return -1;
			}
			ret = adv_transfer(trans_data, trans_count);
			break;
	}
	
	return 0;
}

void dump(char cType, const unsigned char* data,int size)
{
	char c;
	int i,j;
	for(i = 0;i < size;i+=16)
	{
		fprintf(stderr,"%c %04X ",cType,i);
		for(j = 0;j < 16;j++)
		{
			if((i + j) >= size)
			{
				break;
			}
			c = data[i + j];
			fprintf(stderr,"%02X ",(int)c & 0xFF);
			if(j == 7)
			{
			 fprintf(stderr,"- ");
			}
		}
		for(/*j = 0*/;j < 16;j++)
		{
			fprintf(stderr,"   ");
			if(j == 7)
			{
				fprintf(stderr,"- ");
			}
		}
		for(j = 0;j < 16;j++)
		{
			if((i + j) >= size)
			{
				break;
			}
			c = data[i + j];
			fprintf(stderr,"%c",isprint(c) ? c : '?');
			if(j == 7)
			{
				fprintf(stderr," ");
			}
		}
		for(/*j = 0*/;j < 16;j++)
		{
			fprintf(stderr," ");
			if(j == 7)
			{
				fprintf(stderr," ");
			}
		}
		fprintf(stderr,"\n");
	}
	fflush(stderr);
}