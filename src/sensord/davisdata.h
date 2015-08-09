
/* Definition of Davis LOOP data packet */
typedef struct t_RTDATA
{
	unsigned char	yACK;	/* -1 ACK from stream		*/
	unsigned char	cL;	/* 0  unsigned character "L"	*/
	unsigned char	cO;	/* 1  unsigned character "O"	*/
	unsigned char	cO1;	/* 2  unsigned character "O"	*/
	unsigned char	cP;	/* 3  unsigned character "P" (RevA) or the current		*/
				/*    3-hour Barometer trend as follows: 		*/
				/*    196 = Falling Rapidly						*/
				/*    236 = Falling Slowly						*/
				/*    0   = Steady								*/
				/*    20  = Rising Slowly						*/
				/*    60  = Rising Rapidly						*/
				/* any other value is 3-hour data not available */
	unsigned char	yPacketType;	/* 4 Always zero for current firmware release	*/
	unsigned short	wNextRec;	/* 5 loc in archive memory for next data packet	*/
	unsigned short	wBarometer;	/* 7 Current barometer as (Hg / 1000)	*/
	unsigned short	wInsideTemp;	/* 9 Inside Temperature as (DegF / 10)	*/
	unsigned char	yInsideHum;	/* 11 Inside Humidity as percentage	*/
	unsigned short	wOutsideTemp;	/* 12 Outside Temperature as (DegF / 10)*/
	unsigned char	yWindSpeed;	/* 14 Wind Speed			*/
	unsigned char	yAvgWindSpeed;	/* 15 10-Minute Average Wind Speed	*/
	unsigned short	wWindDir;	/* 16 Wind Direction in degress		*/
	unsigned char	yXtraTemps[7];	/* 18 Extra Temperatures		*/
	unsigned char	ySoilTemps[4];	/* 25 Soil Temperatures			*/
	unsigned char	yLeafTemps[4];	/* 29 Leaf Temperatures			*/
	unsigned char	yOutsideHum;	/* 33 Outside Humidity			*/
	unsigned char	yXtraHums[7];	/* 34 Extra Humidities			*/
	unsigned short	wRainRate;	/* 41 Rain Rate				*/
	unsigned char	yUVLevel;	/* 43 UV Level				*/
	unsigned short	wSolarRad;	/* 44 Solar Radiation			*/
	unsigned short	wStormRain;	/* 46 Total Storm Rain			*/
	unsigned short	wStormStart;	/* 48 Start date of current storm	*/
	unsigned short	wRainDay;	/* 50 Rain Today			*/
	unsigned short	wRainMonth;	/* 52 Rain this Month			*/
	unsigned short	wRainYear;	/* 54 Rain this Year			*/
	unsigned short	wETDay;		/* 56 Day ET				*/
	unsigned short	wETMonth;	/* 58 Month ET				*/
	unsigned short	wETYear;	/* 60 Year ET				*/
	unsigned long	wSoilMoist;	/* 62 Soil Moistures			*/
	unsigned long	wLeafWet;	/* 66 Leaf Wetness			*/
	unsigned char	yAlarmInside;	/* 70 Inside Alarm bits			*/
	unsigned char	yAlarmRain;	/* 71 Rain Alarm bits			*/
	unsigned short	yAlarmOut;	/* 72 Outside Temperature Alarm bits	*/
	unsigned char	yAlarmXtra[8];	/* 74 Extra Temp/Hum Alarms		*/
	unsigned long	yAlarmSL;	/* 82 Soil and Leaf Alarms		*/
	unsigned char	yXmitBatt;	/* 86 Transmitter battery status	*/
	unsigned short	wBattLevel;	/* 87 Console Battery Level:		*/
					/*    Voltage = ((wBattLevel * 300)/512)/100.0	*/
	unsigned char	yForeIcon;	/* 89 Forecast Icon			*/
	unsigned char	yRule;		/* 90 Forecast rule number		*/
	unsigned short	wSunrise;	/* 91 Sunrise time (BCD encoded, 24hr)	*/
	unsigned short	wSunset;	/* 93 Sunset time  (BCD encoded, 24hr)	*/
	unsigned char	yLF;		/* 95 Line Feed (\n) 0x0a		*/
	unsigned char	yCR;		/* 96 Carraige Return (\r) 0x0d		*/
	unsigned short	WCRC;		/* 97 CRC check bytes (CCITT-16 standard)		*/
} RTDATA;
