#include "RTC.h"

void kReadRTCTime(BYTE *pbHour, BYTE* pbMinute, BYTE* pbSecond) {
	BYTE bData;
	//시간 읽기
	kOutPortByte(RTC_CMOSADDRESS,RTC_ADDRESS_HOUR);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbHour = RTC_BCDTOBINARY(bData);

	//분 읽기
	kOutPortByte(RTC_CMOSADDRESS,RTC_ADDRESS_MINUTE);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbMinute = RTC_BCDTOBINARY(bData);

	//초 읽기
	kOutPortByte(RTC_CMOSADDRESS,RTC_ADDRESS_SECOND);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbSecond = RTC_BCDTOBINARY(bData);

}

void kReadRTCDate(WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth,
		BYTE* pbDayOfWeek) {
	BYTE bData;

	//년도 읽기
	kOutPortByte(RTC_CMOSADDRESS,RTC_ADDRESS_YEAR);
	bData = kInPortByte(RTC_CMOSDATA);
	*pwYear = RTC_BCDTOBINARY(bData) + 2000;

	//달 읽기
	kOutPortByte(RTC_CMOSADDRESS,RTC_ADDRESS_MONTH);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbMonth = RTC_BCDTOBINARY(bData);

	//일 읽기
	kOutPortByte(RTC_CMOSADDRESS,RTC_ADDRESS_DAYOFMONTH);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbDayOfMonth = RTC_BCDTOBINARY(bData);

	//요일 읽기
	kOutPortByte(RTC_CMOSADDRESS,RTC_ADDRESS_DAYOFWEEK);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbDayOfWeek = RTC_BCDTOBINARY(bData);
}

char* kConvertDayOfWeekToString( BYTE bDayOfWeek )
{
    static char* vpcDayOfWeekString[ 8 ] = { "Error", "Sunday", "Monday",
            "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

    // 요일 범위가 넘어가면 에러를 반환
    if( bDayOfWeek >= 8 )
    {
        return vpcDayOfWeekString[ 0 ];
    }

    // 요일을 반환
    return vpcDayOfWeekString[ bDayOfWeek ];
}
