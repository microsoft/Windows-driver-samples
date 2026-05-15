/******************************************************************************
 *
 * Copyright(c) 2007 - 2012 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/


#ifndef	__RT_CHANNELPLAN_H__
#define __RT_CHANNELPLAN_H__

typedef enum _RT_CHANNEL_DOMAIN_NEW
{

	//===== Add new channel plan above this line ===============//

	// For new architecture we define different 2G/5G CH area for all country.
	// 2.4 G only
	RT_CHANNEL_DOMAIN_2G_WORLD_5G_NULL				= 0x20,
	RT_CHANNEL_DOMAIN_2G_ETSI1_5G_NULL				= 0x21,
	RT_CHANNEL_DOMAIN_2G_FCC1_5G_NULL				= 0x22,
	RT_CHANNEL_DOMAIN_2G_MKK1_5G_NULL				= 0x23,
	RT_CHANNEL_DOMAIN_2G_ETSI2_5G_NULL				= 0x24,
	// 2.4 G + 5G type 1
	RT_CHANNEL_DOMAIN_2G_FCC1_5G_FCC1				= 0x25,
	RT_CHANNEL_DOMAIN_2G_WORLD_5G_ETSI1				= 0x26,
	//RT_CHANNEL_DOMAIN_2G_WORLD_5G_ETSI1				= 0x27,
	// .....

	RT_CHANNEL_DOMAIN_MAX_NEW,

}RT_CHANNEL_DOMAIN_NEW, *PRT_CHANNEL_DOMAIN_NEW;


#if 0
#define DOMAIN_CODE_2G_WORLD \
        {1,2,3,4,5,6,7,8,9,10,11,12,13}, 13
#define DOMAIN_CODE_2G_ETSI1 \
        {1,2,3,4,5,6,7,8,9,10,11,12,13}, 13
#define DOMAIN_CODE_2G_ETSI2 \
        {1,2,3,4,5,6,7,8,9,10,11}, 11
#define DOMAIN_CODE_2G_FCC1 \
        {1,2,3,4,5,6,7,8,9,10,11,12,13,14}, 14
#define DOMAIN_CODE_2G_MKK1 \
        {10,11,12,13}, 4

#define DOMAIN_CODE_5G_ETSI1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140}, 19
#define DOMAIN_CODE_5G_ETSI2 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165}, 24
#define DOMAIN_CODE_5G_ETSI3 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,149,153,157,161,165}, 22
#define DOMAIN_CODE_5G_FCC1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165}, 24
#define DOMAIN_CODE_5G_FCC2 \
        {36,40,44,48,149,153,157,161,165}, 9
#define DOMAIN_CODE_5G_FCC3 \
        {36,40,44,48,52,56,60,64,149,153,157,161,165}, 13
#define DOMAIN_CODE_5G_FCC4 \
        {36,40,44,48,52,56,60,64,149,153,157,161}, 12
#define DOMAIN_CODE_5G_FCC5 \
        {149,153,157,161,165}, 5
#define DOMAIN_CODE_5G_FCC6 \
        {36,40,44,48,52,56,60,64}, 8
#define DOMAIN_CODE_5G_FCC7 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165}, 20
#define DOMAIN_CODE_5G_IC1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165}, 20
#define DOMAIN_CODE_5G_KCC1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,149,153,157,161,165}, 20
#define DOMAIN_CODE_5G_MKK1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140}, 19
#define DOMAIN_CODE_5G_MKK2 \
        {36,40,44,48,52,56,60,64}, 8
#define DOMAIN_CODE_5G_MKK3 \
        {100,104,108,112,116,120,124,128,132,136,140}, 11
#define DOMAIN_CODE_5G_NCC1 \
        {56,60,64,100,104,108,112,116,136,140,149,153,157,161,165}, 24
#define DOMAIN_CODE_5G_NCC2 \
        {56,60,64,149,153,157,161,165}, 8
#define UNDEFINED \
        {0}, 0
#endif

//
//
//
/*

Countries							"Country Abbreviation"	Domain Code					SKU's	Ch# of 20MHz
															2G			5G						Ch# of 40MHz
"Albania�����ڥ���"					AL													Local Test

"Algeria�����ΧQ��"					DZ									CE TCF

"Antigua & Barbuda�w���ʮq&�ڥ��F"	AG						2G_WORLD					FCC TCF

"Argentina���ڧ�"					AR						2G_WORLD					Local Test

"Armenia�Ȭ�����"					AM						2G_WORLD					ETSI

"Aruba���|�ڮq"						AW						2G_WORLD					FCC TCF

"Australia�D�w"						AU						2G_WORLD		5G_ETSI2

"Austria���a�Q"						AT						2G_WORLD		5G_ETSI1	CE

"Azerbaijan�������"				AZ						2G_WORLD					CE TCF

"Bahamas�ګ���"						BS						2G_WORLD

"Barbados�ڤڦh��"					BB						2G_WORLD					FCC TCF

"Belgium��Q��"						BE						2G_WORLD		5G_ETSI1	CE

"Bermuda�ʼ}�F"						BM						2G_WORLD					FCC TCF

"Brazil�ڦ�"						BR						2G_WORLD					Local Test

"Bulgaria�O�[�Q��"					BG						2G_WORLD		5G_ETSI1	CE

"Canada�[���j"						CA						2G_FCC1			5G_FCC7		IC / FCC	IC / FCC

"Cayman Islands�}�Ҹs�q"			KY						2G_WORLD		5G_ETSI1	CE

"Chile���Q"							CL						2G_WORLD					FCC TCF

"China����"							CN						2G_WORLD		5G_FCC5		�H��?�i2002�j353?

"Columbia���ۤ��"					CO						2G_WORLD					Voluntary

"Costa Rica�����F���["				CR						2G_WORLD					FCC TCF

"Cyprus�������"					CY						2G_WORLD		5G_ETSI1	CE

"Czech ���J"						CZ						2G_WORLD		5G_ETSI1	CE

"Denmark����"						DK						2G_WORLD		5G_ETSI1	CE

"Dominican Republic�h�����[�@�M��"	DO						2G_WORLD					FCC TCF

"Egypt�J��"	EG	2G_WORLD			CE T												CF

"El Salvador�ĺ��˦h"				SV						2G_WORLD					Voluntary

"Estonia�R�F����"					EE						2G_WORLD		5G_ETSI1	CE

"Finland����"						FI						2G_WORLD		5G_ETSI1	CE

"France�k��"						FR										5G_E		TSI1	CE

"Germany�w��"						DE						2G_WORLD		5G_ETSI1	CE

"Greece ��þ"						GR						2G_WORLD		5G_ETSI1	CE

"Guam���q"							GU						2G_WORLD

"Guatemala�ʦa����"					GT						2G_WORLD

"Haiti���a"							HT						2G_WORLD					FCC TCF

"Honduras�����Դ�"					HN						2G_WORLD					FCC TCF

"Hungary�I���Q"						HU						2G_WORLD		5G_ETSI1	CE

"Iceland�B�q"						IS						2G_WORLD		5G_ETSI1	CE

"India�L��"							IN						2G_WORLD		5G_FCC3		FCC/CE TCF

"Ireland�R����"						IE						2G_WORLD		5G_ETSI1	CE

"Israel�H��C"						IL										5G_F		CC6	CE TCF

"Italy�q�j�Q"						IT						2G_WORLD		5G_ETSI1	CE

"Japan�饻"							JP						2G_MKK1			5G_MKK1		MKK	MKK

"Korea����"							KR						2G_WORLD		5G_KCC1		KCC	KCC

"Latvia�Բ����"					LV						2G_WORLD		5G_ETSI1	CE

"Lithuania�߳��{"					LT						2G_WORLD		5G_ETSI1	CE

"Luxembourg�c�˳�"					LU						2G_WORLD		5G_ETSI1	CE

"Malaysia���Ӧ��"					MY						2G_WORLD					Local Test

"Malta�����L"						MT						2G_WORLD		5G_ETSI1	CE

"Mexico�����"						MX						2G_WORLD		5G_FCC3		Local Test

"Morocco������"						MA													CE TCF

"Netherlands����"					NL						2G_WORLD		5G_ETSI1	CE

"New Zealand�æ���"					NZ						2G_WORLD		5G_ETSI2

"Norway����"						NO						2G_WORLD		5G_ETSI1	CE

"Panama�ڮ��� "						PA						2G_FCC1						Voluntary

"Philippines��߻�"					PH						2G_WORLD					FCC TCF

"Poland�i��"						PL						2G_WORLD		5G_ETSI1	CE

"Portugal�����"					PT						2G_WORLD		5G_ETSI1	CE

"Romaniaù������"					RO						2G_WORLD		5G_ETSI1	CE

"Russia�Xù��"						RU						2G_WORLD		5G_ETSI3	CE TCF

"Saudi Arabia�F�a���ԧB"			SA						2G_WORLD					CE TCF

"Singapore�s�[�Y"					SG						2G_WORLD

"Slovakia������J"					SK						2G_WORLD		5G_ETSI1	CE

"Slovenia����������"				SI						2G_WORLD		5G_ETSI1	CE

"South Africa�n�D"					ZA						2G_WORLD					CE TCF

"Spain��Z��"						ES										5G_ETSI1	CE

"Sweden���"						SE						2G_WORLD		5G_ETSI1	CE

"Switzerland��h"					CH						2G_WORLD		5G_ETSI1	CE

"Taiwan�O�W"						TW						2G_FCC1			5G_NCC1	NCC

"Thailand����"						TH						2G_WORLD					FCC/CE TCF

"Turkey�g�ը�"						TR						2G_WORLD

"Ukraine�Q�J��"						UA						2G_WORLD					Local Test

"United Kingdom�^��"				GB						2G_WORLD		5G_ETSI1	CE	ETSI

"United States����"					US						2G_FCC1			5G_FCC7		FCC	FCC

"Venezuela�e�����"					VE						2G_WORLD		5G_FCC4		FCC TCF

"Vietnam�V�n"						VN						2G_WORLD					FCC/CE TCF



*/

// Counter abbervation.
typedef enum _RT_COUNTRY_DEFINE_NUM
{
	RT_CTRY_AL,				//	"Albania�����ڥ���"
	RT_CTRY_DZ,             //  "Algeria�����ΧQ��"
	RT_CTRY_AG,             //  "Antigua & Barbuda�w���ʮq&�ڥ��F"
	RT_CTRY_AR,             //  "Argentina���ڧ�"
	RT_CTRY_AM,             //  "Armenia�Ȭ�����"
	RT_CTRY_AW,             //  "Aruba���|�ڮq"
	RT_CTRY_AU,             //  "Australia�D�w"
	RT_CTRY_AT,             //  "Austria���a�Q"
	RT_CTRY_AZ,             //  "Azerbaijan�������"
	RT_CTRY_BS,             //  "Bahamas�ګ���"
	RT_CTRY_BB,             //  "Barbados�ڤڦh��"
	RT_CTRY_BE,             //  "Belgium��Q��"
	RT_CTRY_BM,             //  "Bermuda�ʼ}�F"
	RT_CTRY_BR,             //  "Brazil�ڦ�"
	RT_CTRY_BG,             //  "Bulgaria�O�[�Q��"
	RT_CTRY_CA,             //  "Canada�[���j"
	RT_CTRY_KY,             //  "Cayman Islands�}�Ҹs�q"
	RT_CTRY_CL,             //  "Chile���Q"
	RT_CTRY_CN,             //  "China����"
	RT_CTRY_CO,             //  "Columbia���ۤ��"
	RT_CTRY_CR,             //  "Costa Rica�����F���["
	RT_CTRY_CY,             //  "Cyprus�������"
	RT_CTRY_CZ,             //  "Czech ���J"
	RT_CTRY_DK,             //  "Denmark����"
	RT_CTRY_DO,             //  "Dominican Republic�h�����[�@�M��"
	RT_CTRY_CE,             //  "Egypt�J��"	EG	2G_WORLD
	RT_CTRY_SV,             //  "El Salvador�ĺ��˦h"
	RT_CTRY_EE,             //  "Estonia�R�F����"
	RT_CTRY_FI,             //  "Finland����"
	RT_CTRY_FR,             //  "France�k��"
	RT_CTRY_DE,             //  "Germany�w��"
	RT_CTRY_GR,             //  "Greece ��þ"
	RT_CTRY_GU,             //  "Guam���q"
	RT_CTRY_GT,             //  "Guatemala�ʦa����"
	RT_CTRY_HT,             //  "Haiti���a"
	RT_CTRY_HN,             //  "Honduras�����Դ�"
	RT_CTRY_HU,             //  "Hungary�I���Q"
	RT_CTRY_IS,             //  "Iceland�B�q"
	RT_CTRY_IN,             //  "India�L��"
	RT_CTRY_IE,             //  "Ireland�R����"
	RT_CTRY_IL,             //  "Israel�H��C"
	RT_CTRY_IT,             //  "Italy�q�j�Q"
	RT_CTRY_JP,             //  "Japan�饻"
	RT_CTRY_KR,             //  "Korea����"
	RT_CTRY_LV,             //  "Latvia�Բ����"
	RT_CTRY_LT,             //  "Lithuania�߳��{"
	RT_CTRY_LU,             //  "Luxembourg�c�˳�"
	RT_CTRY_MY,             //  "Malaysia���Ӧ��"
	RT_CTRY_MT,             //  "Malta�����L"
	RT_CTRY_MX,             //  "Mexico�����"
	RT_CTRY_MA,             //  "Morocco������"
	RT_CTRY_NL,             //  "Netherlands����"
	RT_CTRY_NZ,             //  "New Zealand�æ���"
	RT_CTRY_NO,             //  "Norway����"
	RT_CTRY_PA,             //  "Panama�ڮ��� "
	RT_CTRY_PH,             //  "Philippines��߻�"
	RT_CTRY_PL,             //  "Poland�i��"
	RT_CTRY_PT,             //  "Portugal�����"
	RT_CTRY_RO,             //  "Romaniaù������"
	RT_CTRY_RU,             //  "Russia�Xù��"
	RT_CTRY_SA,             //  "Saudi Arabia�F�a���ԧB"
	RT_CTRY_SG,             //  "Singapore�s�[�Y"
	RT_CTRY_SK,             //  "Slovakia������J"
	RT_CTRY_SI,             //  "Slovenia����������"
	RT_CTRY_ZA,             //  "South Africa�n�D"
	RT_CTRY_ES,             //  "Spain��Z��"
	RT_CTRY_SE,             //  "Sweden���"
	RT_CTRY_CH,             //  "Switzerland��h"
	RT_CTRY_TW,             //  "Taiwan�O�W"
	RT_CTRY_TH,             //  "Thailand����"
	RT_CTRY_TR,             //  "Turkey�g�ը�"
	RT_CTRY_UA,             //  "Ukraine�Q�J��"
	RT_CTRY_GB,             //  "United Kingdom�^��"
	RT_CTRY_US,             //  "United States����"
	RT_CTRY_VE,             //  "Venezuela�e�����"
	RT_CTRY_VN,             //  "Vietnam�V�n"
	RT_CTRY_MAX,            //

}RT_COUNTRY_NAME, *PRT_COUNTRY_NAME;

// Scan type including active and passive scan.
typedef enum _RT_SCAN_TYPE_NEW
{
	SCAN_NULL,
	SCAN_ACT,
	SCAN_PAS,
	SCAN_BOTH,
}RT_SCAN_TYPE_NEW, *PRT_SCAN_TYPE_NEW;


// Power table sample.

typedef struct _RT_CHNL_PLAN_LIMIT
{
	u2Byte	Chnl_Start;
	u2Byte	Chnl_end;

	u2Byte	Freq_Start;
	u2Byte	Freq_end;
}RT_CHNL_PLAN_LIMIT, *PRT_CHNL_PLAN_LIMIT;


//
// 2.4G Regulatory Domains
//
typedef enum _RT_REGULATION_DOMAIN_2G
{
	RT_2G_NULL,
	RT_2G_WORLD,
	RT_2G_ETSI1,
	RT_2G_FCC1,
	RT_2G_MKK1,
	RT_2G_ETSI2

}RT_REGULATION_2G, *PRT_REGULATION_2G;


//typedef struct _RT_CHANNEL_BEHAVIOR
//{
//	u1Byte	Chnl;
//	RT_SCAN_TYPE_NEW
//
//}RT_CHANNEL_BEHAVIOR, *PRT_CHANNEL_BEHAVIOR;

//typedef struct _RT_CHANNEL_PLAN_TYPE
//{
//	RT_CHANNEL_BEHAVIOR
//	u1Byte					Chnl_num;
//}RT_CHNL_PLAN_TYPE, *PRT_CHNL_PLAN_TYPE;

//
// 2.4G Channel Number
// Channel definition & number
//
#define CHNL_RT_2G_NULL \
        {0}, 0
#define CHNL_RT_2G_WORLD \
        {1,2,3,4,5,6,7,8,9,10,11,12,13}, 13
#define CHNL_RT_2G_WORLD_TEST \
        {1,2,3,4,5,6,7,8,9,10,11,12,13}, 13

#define CHNL_RT_2G_EFSI1 \
        {1,2,3,4,5,6,7,8,9,10,11,12,13}, 13
#define CHNL_RT_2G_FCC1 \
        {1,2,3,4,5,6,7,8,9,10,11}, 11
#define CHNL_RT_2G_MKK1 \
        {1,2,3,4,5,6,7,8,9,10,11,12,13,14}, 14
#define CHNL_RT_2G_ETSI2 \
        {10,11,12,13}, 4

//
// 2.4G Channel Active or passive scan.
//
#define CHNL_RT_2G_NULL_SCAN_TYPE \
        {SCAN_NULL}
#define CHNL_RT_2G_WORLD_SCAN_TYPE \
        {1,1,1,1,1,1,1,1,1,1,1,0,0}
#define CHNL_RT_2G_EFSI1_SCAN_TYPE \
        {1,1,1,1,1,1,1,1,1,1,1,1,1}
#define CHNL_RT_2G_FCC1_SCAN_TYPE \
        {1,1,1,1,1,1,1,1,1,1,1}
#define CHNL_RT_2G_MKK1_SCAN_TYPE \
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1}
#define CHNL_RT_2G_ETSI2_SCAN_TYPE \
        {1,1,1,1}


//
// 2.4G Band & Frequency Section
// Freqency start & end / band number
//
#define FREQ_RT_2G_NULL \
        {0}, 0
        // Passive scan CH 12, 13
#define FREQ_RT_2G_WORLD \
        {2412, 2472}, 1
#define FREQ_RT_2G_EFSI1 \
        {2412, 2472}, 1
#define FREQ_RT_2G_FCC1 \
        {2412, 2462}, 1
#define FREQ_RT_2G_MKK1 \
        {2412, 2484}, 1
#define FREQ_RT_2G_ETSI2 \
        {2457, 2472}, 1


//
// 5G Regulatory Domains
//
typedef enum _RT_REGULATION_DOMAIN_5G
{
	RT_5G_NULL,
	RT_5G_WORLD,
	RT_5G_ETSI1,
	RT_5G_ETSI2,
	RT_5G_ETSI3,
	RT_5G_FCC1,
	RT_5G_FCC2,
	RT_5G_FCC3,
	RT_5G_FCC4,
	RT_5G_FCC5,
	RT_5G_FCC6,
	RT_5G_FCC7,
	RT_5G_IC1,
	RT_5G_KCC1,
	RT_5G_MKK1,
	RT_5G_MKK2,
	RT_5G_MKK3,
	RT_5G_NCC1,

}RT_REGULATION_5G, *PRT_REGULATION_5G;

//
// 5G Channel Number
//
#define CHNL_RT_5G_NULL \
        {0}, 0
#define CHNL_RT_5G_WORLD \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140}, 19
#define CHNL_RT_5G_ETSI1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165}, 24
#define CHNL_RT_5G_ETSI2 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,149,153,157,161,165}, 22
#define CHNL_RT_5G_ETSI3 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165}, 24
#define CHNL_RT_5G_FCC1 \
        {36,40,44,48,149,153,157,161,165}, 9
#define CHNL_RT_5G_FCC2 \
        {36,40,44,48,52,56,60,64,149,153,157,161,165}, 13
#define CHNL_RT_5G_FCC3 \
        {36,40,44,48,52,56,60,64,149,153,157,161}, 12
#define CHNL_RT_5G_FCC4 \
        {149,153,157,161,165}, 5
#define CHNL_RT_5G_FCC5 \
        {36,40,44,48,52,56,60,64}, 8
#define CHNL_RT_5G_FCC6 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165}, 20
#define CHNL_RT_5G_FCC7 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165}, 20
#define CHNL_RT_5G_IC1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,149,153,157,161,165}, 20
#define CHNL_RT_5G_KCC1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140}, 19
#define CHNL_RT_5G_MKK1 \
        {36,40,44,48,52,56,60,64}, 8
#define CHNL_RT_5G_MKK2 \
        {100,104,108,112,116,120,124,128,132,136,140}, 11
#define CHNL_RT_5G_MKK3 \
        {56,60,64,100,104,108,112,116,136,140,149,153,157,161,165}, 24
#define CHNL_RT_5G_NCC1 \
        {56,60,64,149,153,157,161,165}, 8

//
// 5G Channel Active or passive scan.
//
#define CHNL_RT_5G_NULL_SCAN_TYPE \
        {SCAN_NULL}
#define CHNL_RT_5G_WORLD_SCAN_TYPE \
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
#define CHNL_RT_5G_ETSI1_SCAN_TYPE \
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
#define CHNL_RT_5G_ETSI2_SCAN_TYPE \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,149,153,157,161,165}, 22
#define CHNL_RT_5G_ETSI3_SCAN_TYPE \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165}, 24
#define CHNL_RT_5G_FCC1_SCAN_TYPE \
        {36,40,44,48,149,153,157,161,165}, 9
#define CHNL_RT_5G_FCC2_SCAN_TYPE \
        {36,40,44,48,52,56,60,64,149,153,157,161,165}, 13
#define CHNL_RT_5G_FCC3_SCAN_TYPE \
        {36,40,44,48,52,56,60,64,149,153,157,161}, 12
#define CHNL_RT_5G_FCC4_SCAN_TYPE \
        {149,153,157,161,165}, 5
#define CHNL_RT_5G_FCC5_SCAN_TYPE \
        {36,40,44,48,52,56,60,64}, 8
#define CHNL_RT_5G_FCC6_SCAN_TYPE \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165}, 20
#define CHNL_RT_5G_FCC7_SCAN_TYPE \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165}, 20
#define CHNL_RT_5G_IC1_SCAN_TYPE \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,149,153,157,161,165}, 20
#define CHNL_RT_5G_KCC1_SCAN_TYPE \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140}, 19
#define CHNL_RT_5G_MKK1_SCAN_TYPE \
        {36,40,44,48,52,56,60,64}, 8
#define CHNL_RT_5G_MKK2_SCAN_TYPE \
        {100,104,108,112,116,120,124,128,132,136,140}, 11
#define CHNL_RT_5G_MKK3_SCAN_TYPE \
        {56,60,64,100,104,108,112,116,136,140,149,153,157,161,165}, 24
#define CHNL_RT_5G_NCC1_SCAN_TYPE \
        {56,60,64,149,153,157,161,165}, 8

//
// Global Regulation
//
typedef enum _RT_REGULATION_COMMON
{
	RT_WORLD,
	RT_FCC,
	RT_MKK,
	RT_ETSI,
	RT_IC,
	RT_CE,
	RT_NCC,

}RT_REGULATION_CMN, *PRT_REGULATION_CMN;



//
// Special requirement for different regulation domain.
// For internal test or customerize special request.
//
typedef enum _RT_CHNLPLAN_SREQ
{
	RT_SREQ_NA						= 0x0,
	RT_SREQ_2G_ADHOC_11N			= 0x00000001,
	RT_SREQ_2G_ADHOC_11B			= 0x00000002,
	RT_SREQ_2G_ALL_PASS				= 0x00000004,
	RT_SREQ_2G_ALL_ACT				= 0x00000008,
	RT_SREQ_5G_ADHOC_11N			= 0x00000010,
	RT_SREQ_5G_ADHOC_11AC			= 0x00000020,
	RT_SREQ_5G_ALL_PASS				= 0x00000040,
	RT_SREQ_5G_ALL_ACT				= 0x00000080,
	RT_SREQ_C1_PLAN					= 0x00000100,
	RT_SREQ_C2_PLAN					= 0x00000200,
	RT_SREQ_C3_PLAN					= 0x00000400,
	RT_SREQ_C4_PLAN					= 0x00000800,
	RT_SREQ_NFC_ON					= 0x00001000,
	RT_SREQ_MASK					= 0x0000FFFF,   /* Requirements bit mask */

}RT_CHNLPLAN_SREQ, *PRT_CHNLPLAN_SREQ;


//
// RT_COUNTRY_NAME & RT_REGULATION_2G & RT_REGULATION_5G transfer table
//
//
typedef struct _RT_CHANNEL_PLAN_COUNTRY_TRANSFER_TABLE
{
	//
	// Define countery domain and corresponding
	//
	RT_COUNTRY_NAME		Country_Enum;
	char				Country_Name[3];

	//char		Domain_Name[12];
	RT_REGULATION_2G	Domain_2G;

	RT_REGULATION_5G	Domain_5G;

	RT_CHANNEL_DOMAIN	RtChDomain;
	//u1Byte		Country_Area;

}RT_CHNL_CTRY_TBL, *PRT_CHNL_CTRY_TBL;


#define		RT_MAX_CHNL_NUM_2G		13
#define		RT_MAX_CHNL_NUM_5G		44

// Power table sample.

typedef struct _RT_CHNL_PLAN_PWR_LIMIT
{
	u2Byte	Chnl_Start;
	u2Byte	Chnl_end;
	u1Byte	dB_Max;
	u2Byte	mW_Max;
}RT_CHNL_PWR_LIMIT, *PRT_CHNL_PWR_LIMIT;


#define		RT_MAX_BAND_NUM			5

typedef struct _RT_CHANNEL_PLAN_MAXPWR
{
//	STRING_T
	RT_CHNL_PWR_LIMIT	Chnl[RT_MAX_BAND_NUM];
	u1Byte				Band_Useful_Num;


}RT_CHANNEL_PLAN_MAXPWR, *PRT_CHANNEL_PLAN_MAXPWR;


//
// Power By Rate Table.
//



typedef struct _RT_CHANNEL_PLAN_NEW
{
	//
	// Define countery domain and corresponding
	//
	//char		Country_Name[36];
	//u1Byte		Country_Enum;

	//char		Domain_Name[12];


	PRT_CHNL_CTRY_TBL		pCtryTransfer;

	RT_CHANNEL_DOMAIN		RtChDomain;

	RT_REGULATION_2G		Domain_2G;

	RT_REGULATION_5G		Domain_5G;

	RT_REGULATION_CMN		Regulator;

	RT_CHNLPLAN_SREQ		ChnlSreq;

	//RT_CHNL_PLAN_LIMIT		RtChnl;

	u1Byte	Chnl2G[MAX_CHANNEL_NUM];				// CHNL_RT_2G_WORLD
	u1Byte	Len2G;
	u1Byte	Chnl2GScanTp[MAX_CHANNEL_NUM];			// CHNL_RT_2G_WORLD_SCAN_TYPE
	//u1Byte	Freq2G[2];								// FREQ_RT_2G_WORLD

	u1Byte	Chnl5G[MAX_CHANNEL_NUM];
	u1Byte	Len5G;
	u1Byte	Chnl5GScanTp[MAX_CHANNEL_NUM];
	//u1Byte	Freq2G[2];								// FREQ_RT_2G_WORLD

	RT_CHANNEL_PLAN_MAXPWR	ChnlMaxPwr;


}RT_CHANNEL_PLAN_NEW, *PRT_CHANNEL_PLAN_NEW;


#endif	// __RT_CHANNELPLAN_H__





