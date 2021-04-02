#include "fingerprintWrapper.h"

FingerBoard::FingerBoard(HardwareSerial* hs)
{
	sensor = Fingerprint(&Serial1);
	sensor.begin(57600);

	sensorSerial = hs;

	Keyboard.begin();
}

FbStatus FingerBoard::Begin()
{
	FbStatus ret = FbStatus::SUCCESS;

	if (sensor.verifyPassword())
		ret = FbStatus::SUCCESS;
	else
		ret = FbStatus::ERROR_SENSOR_NOT_FOUND;

	return ret;
}

FbStatus FingerBoard::Begin(Serial_* hs)
{
	debug = hs;

	return Begin();
}

bool FingerBoard::CmdCheck()
{
	int p;
	String pswd;

	while (Serial.available() > 0)
	{
		char nullByte = char(Serial.read());
		if (nullByte == '\n')
		{
			comdata[data_p] = nullByte;
			data_p = 0;

			switch (comdata[0])
			{
			case 'A':
				strtok(comdata, ",");
				p = atoi(strtok(NULL, ","));

				while (true)
				{
					Serial.println("Please enter the ID you want to enroll:");

					if (p >= 0 && p < MAX_FINGERS)
					{
						Serial.print("Enrolling [");
						Serial.print(p);
						Serial.println("] finger...");

						if (AddFinger(p))
						{
							Serial.println("-------------------");
							Serial.println("Add ok !");
							Serial.println("-------------------");

							delay(2000);
						}
						else
						{
							Serial.println("-------------------");
							Serial.println("Add failed !");
							Serial.println("-------------------");

							delay(2000);
						}

						break;
					}
					else
					{
						Serial.println("Please enter a valid ID !");
					}
				}

				break;

			case 'P': // 2021.1 by Hcreak
				strtok(comdata, ",");
				p = atoi(strtok(NULL, ","));

				pswd = String(strtok(NULL, ";"));
				passwords[p] = pswd;

				Serial.print("Set NO.");
				Serial.print(p);
				Serial.print(" Password to [");
				Serial.print(pswd);
				Serial.println("]");

				saveConfig();

				break;

			case 'D':
				DeleteAllFingers();
				Serial.println("All Finger deleted !");

				for(int i = 0; i < MAX_FINGERS; i++)
				{
					passwords[i] = "";
				}
				clearConfig();
				Serial.println("All Password deleted !");

				break;

			case 'T':
				// sensor.getTemplateCount();
				// Serial.print("T_number:  ");
				// Serial.println(sensor.templateCount);
	
				Serial.print("password_0:  ");
				Serial.println(passwords[0]);
				// loadConfig();

				// Serial.print("EEPROM_0:  ");
				// Serial.println(EEPROM.read(0));

				break;

			case 'F':
				Serial.print("free:  ");
				Serial.println(FPMXX_Get_Free_Position());

			}
		}
		else
		{
			comdata[data_p] = nullByte;
			data_p++;
		}
	}

	return true;
}

int FingerBoard::GetFingerID()
{
	if (millis() - time > INTERVAL)
	{
		time = millis();

		uint8_t p = sensor.getImage();

		switch (p)
		{
		case FINGERPRINT_OK:
			if (debug)
				debug->println("Image taken");
			break;
		case FINGERPRINT_NOFINGER:
			isTouching = false;
			return -1;
		case FINGERPRINT_PACKETRECIEVEERR:
			Serial.println("Communication error");
			return -1;
		case FINGERPRINT_IMAGEFAIL:
			if (debug)
				debug->println("Imaging error");
			return -1;
		default:
			if (debug)
				debug->println("Unknown error");
			return -1;
		}

		// OK success!

		p = sensor.image2Tz();
		switch (p)
		{
		case FINGERPRINT_OK:
			if (debug)
				debug->println("Image converted");
			break;
		case FINGERPRINT_IMAGEMESS:
			if (debug)
				debug->println("Image too messy");
			return -1;
		case FINGERPRINT_PACKETRECIEVEERR:
			if (debug)
				debug->println("Communication error");
			return -1;
		case FINGERPRINT_FEATUREFAIL:
			if (debug)
				debug->println("Could not find fingerprint features");
			return -1;
		case FINGERPRINT_INVALIDIMAGE:
			if (debug)
				debug->println("Could not find fingerprint features");
			return -1;
		default:
			if (debug)
				debug->println("Unknown error");
			return -1;
		}

		// OK converted!
		p = sensor.fingerFastSearch();
		if (p == FINGERPRINT_OK)
		{
			if (debug)
				debug->println("Found a print match!");
		}
		else if (p == FINGERPRINT_PACKETRECIEVEERR)
		{
			if (debug)
				debug->println("Communication error");
			return -1;
		}
		else if (p == FINGERPRINT_NOTFOUND)
		{
			if (debug)
				debug->println("Did not find a match");
			return -1;
		}
		else
		{
			if (debug)
				debug->println("Unknown error");
			return -1;
		}

		// found a match!
		if (debug)
		{
			debug->print("Found ID #");
			debug->print(sensor.fingerID);

			debug->print(" with confidence of ");
			debug->println(sensor.confidence);
		}
	}
	else
		return -1;

	return sensor.fingerID;
}

bool FingerBoard::AddFinger(unsigned int id)
{
	int FAIL_COUNT = 10;
	while (FAIL_COUNT--)
	{
		if (FPMXX_Add_Fingerprint(id))
			return true;

		delay(1000);
	}

	return false;
}

void FingerBoard::DeleteAllFingers()
{
	FPMXX_Delete_All_Fingerprint();
}

void FingerBoard::InputPassword(int fingerid)
{
	if (!isTouching)
	{
		Keyboard.println(passwords[fingerid]);
		Keyboard.write(176); //ENTER

		isTouching = true;
	}
}

void FingerBoard::TypeString(String s, bool enter)
{
	Keyboard.println(s);
	if (enter)
		Keyboard.write(176);
}

void FingerBoard::PressKey(uint8_t k)
{
	Keyboard.press(k);
}

void FingerBoard::ReleaseAll()
{
	Keyboard.releaseAll();
}


// Sensor drivers

void FingerBoard::FPMXX_Cmd_Save_Finger(unsigned int storeID)
{
	unsigned long temp = 0;
	unsigned char i;

	FPMXX_Save_Finger[5] = (storeID & 0xFF00) >> 8;
	FPMXX_Save_Finger[6] = (storeID & 0x00FF);

	for (i = 0; i < 7; i++) //计算校验和
		temp = temp + FPMXX_Save_Finger[i];

	FPMXX_Save_Finger[7] = (temp & 0x00FF00) >> 8; //存放校验数据
	FPMXX_Save_Finger[8] = temp & 0x0000FF;

	FPMXX_Send_Cmd(9, FPMXX_Save_Finger, 12);
}

void FingerBoard::FPMXX_Send_Cmd(unsigned char length, unsigned char* address, unsigned char returnLength)
{
	unsigned char i = 0;

	sensorSerial->flush();

	for (i = 0; i < 6; i++) //包头
	{
		sensorSerial->write(FPMXX_Pack_Head[i]);
	}

	for (i = 0; i < length; i++)
	{
		sensorSerial->write(*address);
		address++;
	}

	FPMXX_Cmd_Receive_Data(returnLength);
}

void FingerBoard::FPMXX_Cmd_Receive_Data(unsigned int r_size)
{
	int i = 0;

	while (1)
	{
		if (sensorSerial->available() == r_size)
		{
			for (i = 0; i < r_size; i++)
			{
				FPMXX_RECEIVE_BUFFER[i] = sensorSerial->read();
			}
			break;
		}
	}
}

void FingerBoard::FPMXX_Cmd_StoreTemplate(unsigned int ID)
{
	unsigned int temp = 0;

	FPMXX_Save_Finger[5] = (ID & 0xFF00) >> 8;
	FPMXX_Save_Finger[6] = (ID & 0x00FF);

	for (int i = 0; i < 7; i++) //计算校验和
		temp = temp + FPMXX_Save_Finger[i];

	FPMXX_Save_Finger[7] = (temp & 0x00FF00) >> 8; //存放校验数据
	FPMXX_Save_Finger[8] = temp & 0x0000FF;

	sensorSerial->write((char*)FPMXX_Pack_Head, 6);
	sensorSerial->write((char*)FPMXX_Save_Finger, 9);

	FPMXX_Cmd_Receive_Data(12);
}

bool FingerBoard::FPMXX_Add_Fingerprint(unsigned int  writeID)
{
	FPMXX_Send_Cmd(6, FPMXX_Get_Img, 12);

	//判断接收到的确认码,等于0指纹获取成功    
	if (FPMXX_RECEIVE_BUFFER[9] == 0)
	{
		delay(100);
		FPMXX_Send_Cmd(7, FPMXX_Img_To_Buffer1, 12); //发送命令 将图像转换成 特征码 存放在 CHAR_buffer1

		while (1)
		{
			FPMXX_Send_Cmd(6, FPMXX_Get_Img, 12);

			//判断接收到的确认码,等于0指纹获取成功   
			if (FPMXX_RECEIVE_BUFFER[9] == 0)
			{
				delay(500);
				FPMXX_Send_Cmd(7, FPMXX_Img_To_Buffer2, 12); //发送命令 将图像转换成 特征码 存放在 CHAR_buffer1
				FPMXX_Cmd_StoreTemplate(writeID);

				return true;
			}

			delay(100);
		}
	}

	return false;
}

void FingerBoard::FPMXX_Delete_All_Fingerprint()
{
	//添加清空指纹指令
	FPMXX_Send_Cmd(6, FPMXX_Delete_All_Model, 12); //发送命令 将图像转换成 特征码 存放在 CHAR_buffer1

	delay(2000);
}

int FingerBoard::FPMXX_Get_Free_Position() //不做字典暂时用不到
{
	FPMXX_Send_Cmd(7, FPMXX_List_Library, 44); //列出指纹库占用情况

	int i,x;

	for(i = 0; i < MAX_FINGERS/8; i++) //数量判断有问题
	{
		byte byte_block = FPMXX_RECEIVE_BUFFER[10+i];
		for(x = 0; x < 8; x++)
		{
			if(bitRead(byte_block,x) == 0)
			{
				int position = (i*8)+x;
				return position;
			}
		}
	}

	return -1;
	// 缺少超出库容判断
}

// 保存参数到EEPROM
void FingerBoard::saveConfig()
{
	clearConfig();
	
	// char config[MAX_FINGERS][MAX_PASSWORD_LENGTH];
	// for(int i = 0; i < MAX_FINGERS; i++)
	// {
	// 	strcpy(config[i], passwords[i].c_str());
	// }

	// uint8_t *p = (uint8_t*)(&config);
	// for(int i = 0; i < sizeof(config); i++)
	// {
	// 	EEPROM.write(i, *(p + i));
	// }

	int addrOffset = 0;

	for(int i = 0; i < MAX_FINGERS; i++)
	{
		String strToWrite = passwords[i];
		byte len = strToWrite.length();
		EEPROM.write(addrOffset, len);
		for (int x = 0; x < len; x++)
		{
			EEPROM.write(addrOffset + 1 + x, strToWrite[x]);
		}
		addrOffset = addrOffset + 1 + len;
	}
}

// 从EEPROM加载参数
void FingerBoard::loadConfig()
{
	// char config[MAX_FINGERS][MAX_PASSWORD_LENGTH];
	// uint8_t *p = (uint8_t*)(&config);
	// for(int i = 0; i < sizeof(config); i++)
	// {
	// 	*(p + i) = EEPROM.read(i);
	// 	// Serial.println(*(p + i));
	// }

	// for(int i = 0; i < MAX_FINGERS; i++)
	// {	
	// 	config[i][MAX_PASSWORD_LENGTH-1] = '\0';
	// 	passwords[i] = String(config[i]);
	// 	Serial.println(passwords[i]);
	// 	// for(int x = 0; x < MAX_PASSWORD_LENGTH; x++)
	// 	// {
	// 	// 	passwords[i] = config[i];
	// 	// }
	// }

	int addrOffset = 0;

	for(int i = 0; i < MAX_FINGERS; i++)
	{
		int newStrLen = EEPROM.read(addrOffset);
		char data[newStrLen + 1];
		for (int x = 0; x < newStrLen; x++)
		{
			data[x] = EEPROM.read(addrOffset + 1 + x);
		}
		data[newStrLen] = '\0'; // the character may appear in a weird way, you should read: 'only one backslash and 0'
		passwords[i] = String(data);
		addrOffset = addrOffset + 1 + newStrLen;
	}
}

// 清空EEPROM
void FingerBoard::clearConfig()
{
	for(int i = 0 ; i < EEPROM.length() ; i++) 
	{
    	EEPROM.write(i, 0);
  	}
}