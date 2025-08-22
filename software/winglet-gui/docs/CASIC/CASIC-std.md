![[CASIC_ProtocolSpecification.pdf]]

# 1 NMEA
## 1.1 NMEA Standard

Supports NMEA0183 4.1, compatible with 2.3 through 3.x, Supports 4.0 standard, and standards before 2.3

8 bit data words, with a starting bit and a parity bit.

## Serial standard Setup
Baud Rates 4800, 9600, 19200, 38400, 57600, 115200
Data Bytes 8
Stop Bit 1
Parity Bit No

## 1.3 NMEA Identifier and Field Type

### 1.3.1 Satellite Identification (Sender Type)
The NMEA statement distinguishes different GNSS modes through the sender identifier. The sender identifier is defined as follows:

| Transmitter ( GNSS Constellation)           | Identifier |
| ------------------------------------------- | ---------- |
| BeiDou Navigation Satellite System (BDS)    | BD         |
| Global Positioning System (GPS, SBAS, QZSS) | GP         |
| GLONASS                                     | GL         |
| All GNSS Constellations                     | GN         |
| Custom Information                          | P          |
### 1.3.2 Satellite number identifier
Satellite system NMEA satellite number identifier satellite PRN number satellite number corresponds to its PRN

| Satellite System | NMEA Satellite Number | Satellite PRN | Relation to PRN |
| ---------------- | --------------------- | ------------- | --------------- |
| GPS              | 1 -32                 | 1 - 32        | 0 + PRN         |
| SBAS             | 33 - 51               | 120 - 138     | PRN - 87        |
| GLONASS          | 65 - 88               | 1 - 24        | PRN - 64        |
| BDS              | 1 - 37                | 1 - 37        | 0 + PRN         |
| QZSS             | 33 - 37               | 193 - 197     | PRN - 160       |
### 1.3.3 System Information
The CASIC receiver supports a variety of NMEA data protocol formats. The differences between different protocols are reflected in the system identifier. At the same time, the new version of the protocol adds some fields.

|     | NMEA 4.0 and Below | NMEA 4.1                                                   |
| --- | ------------------ | ---------------------------------------------------------- |
| GGA | (1)                | (1)                                                        |
| ZDA | (1)                | (1)                                                        |
| GLL | (1)                | (1)                                                        |
| RMC | (1)                | (1)                                                        |
| VTG | (1)                | (1)                                                        |
| GSA | (2)                | (1) Add additional fields to distinguish different systems |
| GSV | (2)                | (2)                                                        |
Notes:
(1) Identification: If only BD, GPS, GLONASS, Galileo and other satellites are used for location calculation, the transmission identifiers are BD, GP, GL, GA, etc. If satellites of multiple systems are used to obtain location calculation, the transmission identifier is GN
(2)Identification: GP (GPS satellite), BD (BDS satellite), GL (GLONASS satellite)

Regarding Section 1.1, the CASIC receiver supports three versions of the NMEA0183 protocol standard. The differences between these three standards are listed as follows. The main differences between NMEA2.2 and 2.3/4.0 are:
1) The positioning mode (Mode) in GLL, RMC and VTG statements is not output. 
2) In the positioning quality (FS) item in the GGA statement, both trace esculation and normal positioning use 1 (set the trace calculation to 6 in 2.3). 

The NMEA 4.1 protocol adds some fields on the basis of 4.0: 
1) Add systemId to the GSA statement. 
2) Add signalId to the GSV statement. 
3) Add navStatus to the RMC statement. For details, please refer to the introduction of NMEA statements in Section 1.5.
### 1.3.4 Field Types

Dedicated Format Types

| Field Type     | Symbol     | Description                                                                                                                                                                                                                                                                                             |
| -------------- | ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| State          | A          | Single-character field: <br>A=Yes, data is valid, alarm flag cleared<br>V=No, data invalid, alarm flag setting                                                                                                                                                                                          |
| Latitude       | ddmm.mmmm  | The fixed/variable length field **dd** represents the degree of the fixed length of 2<br>The **mm** table before the decimal point indicates the fraction of the fixed length of 2<br>The **mmmm** after the decimal point represents the variable decimal point.                                       |
| Longitude      | dddmm.mmmm | The fixed/variable length field **ddd** represents the degree of the fixed length of 3 <br>the **mm** before the decimal point represents the fraction of the fixed length of 2<br>The **mmmm** after the decimal point represents the fraction of the variable length.                                 |
| Time           | hhmmss.sss | The fixed length field **hh** represents hours with a fixed length of 2<br>**mm** represents a minute with a fixed length of 2<br>**ss** before the decimal point represents a second with a fixed length of 2<br>**sss** after the decimal point represents a decimal second with a fixed length of 3. |
| Constant Field |            | Some fields specify the constants used in predefined.                                                                                                                                                                                                                                                   |
Numerical field

| Field type                 | Symbol | Description                                                                              |
| -------------------------- | ------ | ---------------------------------------------------------------------------------------- |
| Variable numbers           | x.x    | Variable length or floating-point number field                                           |
| Fixed hexadecimal fields   | hh___  | A fixed-length hexadecimal number, the highest effective bit is on the left.             |
| Variable hexadecimal field | h--h   | A hexadecimal number with variable length, and the highest effective bit is on the left. |
Information Field

| Field Type           | Symbol | Description                                          |
| -------------------- | ------ | ---------------------------------------------------- |
| Fixed alphabet field | aa___  | Fixed-length uppercase or lowercase character fields |
| Fixed number field   | xx___  | Fixed-length digital character field                 |
| Variable text        | c--c   | Valid character field of variable length             |
## 1.4 NMEA Messages Overview

Note: Some of this infomation is supplemental from the [[NMEA_Reference_Manual-Rev2.1-Dec07.pdf]] datasheet from SiRF and from [Trimble's Help Documentation](https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_MessageOverview.html)

| NMEA Standard Mesages | Class / ID | Description                                                                             |
| --------------------- | ---------- | --------------------------------------------------------------------------------------- |
| GGA                   | 0x4E 0x00  | Time, position and fix type data                                                        |
| GLL                   | 0x4E 0x01  | Latitude, longitude, UTC time of position fix and status                                |
| GSA                   | 0x4E 0x02  | GPS receiver operating mode, satellites used in the position solution, and DOP values   |
| GSV                   | 0x4E 0x03  | Number of GPS satellites in view satellite ID numbers, elevation, azimuth, & SNR values |
| RMC                   | 0x4E 0x04  | Time, date, position, course and speed data                                             |
| VTG                   | 0x4E 0x05  | Course and speed information relative to the ground                                     |
| GST                   | 0x4E 0x07  | Position error statistics                                                               |
| ZDA                   | 0x4E 0x08  | PPS timing message (synchronized to PPS)                                                |
| ANT (1)               | 0x4E 0x11  | Antenna status                                                                          |
| LPS (1)               | 0x4E 0x12  | Satellite system leap second correction information                                     |
| DHV (1)               | 0x4E 0x13  | Receiver speed information                                                              |
| UTC (1)               | 0x4E 0x16  | Simplified receiver status and leap second correction                                   |
Note: (1) These don't seem to be NMEA standard messages at all. I was unable to find any other manufacturers that used these messages. These messages will be translated for use.

| NMEA CASIC Custom Extensions | Description                                                      |
| ---------------------------- | ---------------------------------------------------------------- |
| CAS00                        | Save the configuration information                               |
| CAS01                        | Communication protocol and serial port configuration information |
| CAS02                        | Set the location update rate                                     |
| CAS03                        | Enable or prohibit the output of information and its frequency   |
| CAS04                        | Set the initialization system and the number of channels         |
| CAS05                        | Set the sender identifier of the NMEA statement                  |
| CAS06                        | Query module software and hardware information                   |
| CAS10                        | Startup mode and auxiliary information configuration             |
| CAS12                        | Standby mode control                                             |
| CAS20                        | Online Upgrade Instruction C                                     |
## 1.5 NMEA Standard

### 1.5.1 GGA

| Information | INS                                                                                             |
| ----------- | ----------------------------------------------------------------------------------------------- |
| Description | Global Positioning System Fixed Data                                                            |
| Type        | Output                                                                                          |
| Format      | \$--GGA,UTCtime,lat,uLat,lon,uLon,FS,numSv,HDOP,msl,uMsl,sep,uSep,diffAge,diffSta\*CS\<CR>\<LF> |
| Example     | \$GPGGA,235316.000,2959.9925,S,12000.0090,E,1,06,1.21,62.77,M,0.00,M,,\*7B                      |

| Field | Text       | Format            | Description                                                                                                                                                                                                                |
| ----- | ---------- | ----------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$--GGA    |                   |                                                                                                                                                                                                                            |
| 2     | UTCtime    | hhmmss.sss        | UTC time of the current location                                                                                                                                                                                           |
| 3     | lat        | ddmm.mmmm         | Latitude, the first 2 characters represent the degree, and the following characters represent the division.                                                                                                                |
| 4     | uLat       | Character         | Latitude direction: N-north, S-south                                                                                                                                                                                       |
| 5     | lon        | dddmm.mmmm        | Longitude, the first 3 characters represent the degree, and the following characters represent the score                                                                                                                   |
| 6     | uLon       | Character         | Longitude direction: E-East, W-West                                                                                                                                                                                        |
| 7     | FS         | Numerical Value   | Indicates the current positioning quality (Note 1), and the field should not be empty.                                                                                                                                     |
| 8     | numSv      | Numerical Value   | The number of satellites used for positioning, 00~24                                                                                                                                                                       |
| 9     | HDOP       | Numerical Value   | Horizontal Accuracy Factor (HDOP)                                                                                                                                                                                          |
| 10    | msl        | Numerical Value   | Altitude, that is, the height of the receiver antenna relative to the ground level                                                                                                                                         |
| 11    | uMsl       | Character         | Height unit, meters, fixed character M                                                                                                                                                                                     |
| 12    | sep        | Numerical Value   | Geoid-to-ellipsoid separation.<br>The distance between the reference ellipsoid and the geodespheric plane, the level plane is lower than the reference ellipsoid.<br>Ellipsoid altitude = MSL Altitude + Geoid Separation. |
| 13    | uSep       | Character         | Separation unit, meters, fixed character M                                                                                                                                                                                 |
| 14    | diffAge    | Numerical Value   | Differentially modified data age, the field is empty when DGPS is not used.                                                                                                                                                |
| 15    | diffSta    | Numerical Value   | The ID of the differential reference station, if used.                                                                                                                                                                     |
| 16    | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                                                                                               |
| 17    | \<CR>\<LF> | Character         | Carriage Return and Line Feed                                                                                                                                                                                              |

| Note 1 Value | Position Quality Description                                         |
| ------------ | -------------------------------------------------------------------- |
| 0            | Location is unavailable or invalid                                   |
| 1            | GPS SPS positioning mode, effective positioning                      |
| 2\*          | Differential GPS, SPS mode, fix valid                                |
| 3-5\*        | Not supported                                                        |
| 6            | Dead Reckoning Mode, fix valid. (Only valid for NMEA 2.3 and above.) |
Note: \* These items were taken from [[NMEA_Reference_Manual-Rev2.1-Dec07.pdf]]
### 1.5.2 GLL

| Information | GLL                                                         |
| ----------- | ----------------------------------------------------------- |
| Description | Geographic Position - Latitude/Longitude                    |
| Type        | Output                                                      |
| Format      | \$--GLL,lat,uLat,lon,uLon, UTCtime,valid,mode\*CS\<CR>\<LF> |
| Example     | \$GPGLL,2959.9925,S,12000.0090,E,235316.000,A,A\*4E         |

| Field | Text       | Format            | Description                                                                                                 |
| ----- | ---------- | ----------------- | ----------------------------------------------------------------------------------------------------------- |
| 1     | \$--GLL    |                   |                                                                                                             |
| 2     | lat        | ddmm.mmmm         | Latitude, the first 2 characters represent the degree, and the following characters represent the division. |
| 3     | uLat       | Character         | Latitude direction: N-north, S-south                                                                        |
| 4     | lon        | dddmm.mmmm        | Longitude, the first 3 characters represent the degree, and the following characters represent the score    |
| 5     | uLon       | Character         | Longitude direction: E-East, W-West                                                                         |
| 6     | UTCtime    | hhmmss.sss        | UTC time of the current location                                                                            |
| 7     | valid      | Character         | Data Validity (Note 1)                                                                                      |
| 8     | mode       | Character         | Positioning mode (Note 2), only NMEA2.3 and the above versions are valid                                    |
| 9     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                |
| 10    | \<CR>\<LF> | Character         | Carriage Return and Line Feed                                                                               |

| (Note 1) Data Validity | Description          |
| ---------------------- | -------------------- |
| A                      | The data is valid.   |
| V                      | The data is invalid. |

| (Note 2) Positioning modes | Description                                                          |
| -------------------------- | -------------------------------------------------------------------- |
| A                          | Autonomous Mode                                                      |
| E                          | Dead Reckoning Mode (Only NMEA v3.00)                                |
| N                          | The data is invalid                                                  |
| D                          | DGPS Mode                                                            |
| M                          | No location, but there is an external input or saved prior location. |
### 1.5.3 GSA

| Information | GSA                                                    |
| ----------- | ------------------------------------------------------ |
| Description | GNSS DOP and Active Satellites                         |
| Type        | Output                                                 |
| Format      | \$--GSA,smode,FS{,SVID},PDOP,HDOP,VDOP\*CS\<CR>\<LF>   |
| Example     | \$GPGSA,A,3,05,21,31,12,18,29,,,,,,,2.56,1.21,2.25\*01 |
Satellite number and Dilution of Precision (DOP) information used for positioning. Whether it is located or whether there is an available satellite, the GSA statement is output; when the receiver is working jointly with multiple GNSS systems, the available satellites of each system corresponds to a GSA statement, and each GSA statement contains Position DOP (PDOP), Horizontal DOP (HDOP) and Vertical DOP (VDOP).

| Field | Text       | Format            | Description                                                                                                                                                                                                                    |
| ----- | ---------- | ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 1     | \$--GSA    |                   |                                                                                                                                                                                                                                |
| 2     | smode      | Character         | Mode switching mode indication (Note 1)                                                                                                                                                                                        |
| 3     | FS         | Number            | Positioning status mark (Note 2)                                                                                                                                                                                               |
| 4     | {,SVID}    | Numerical Value   | The satellite number used for positioning, this field displays a total of 12 available satellite numbers. When there are more than 12, only the first 12 are output, and when there are less than 12, the extra area is empty. |
| 5     | PDOP       | Numerical Value   | Position Accuracy Factor (PDOP)                                                                                                                                                                                                |
| 6     | HDOP       | Numerical Value   | Horizontal Accuracy Factor (HDOP)                                                                                                                                                                                              |
| 7     | VDOP       | Numerical Value   | Vertical Accuracy Factor (VDOP)                                                                                                                                                                                                |
| 8     | systemId   | Numerical Value   | The GNSS system ID number (Note 3) defined by NMEA is only valid for NMEA4.1 and above versions.                                                                                                                               |
| 9     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                                                                                                   |
| 10    | \<CR>\<LF> | Character         | Carriage Return and Line Feed                                                                                                                                                                                                  |

| (Note 1) Mode Indication | Description                                                                    |
| ------------------------ | ------------------------------------------------------------------------------ |
| M                        | Manual switching. Forced to 2D or 3D working mode                              |
| A                        | 2D Automatic switching. The receiver automatically switches 2D/3D working mode |

| (Note 2) Positioning Status | Description                  |
| --------------------------- | ---------------------------- |
| 1                           | Position is Invalid          |
| 2                           | 2D Positioning (<4 SVs used) |
| 3                           | 3D Positioning (>3 SVs used) |

| (Note 3) System ID | Description |
| ------------------ | ----------- |
| 1                  | GPS         |
| 2                  | GLONASS     |
| 4                  | Beidou      |
### 1.5.4 GSV

| Information | GSV                                                                                                                                                                                               |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Description | GNSS Satellites in View<br>The number of {satellite number, elevation, azimuth, carrier-to-noise ratio} parameter groups in each GSV statement is variable, up to 4 groups and at least 0 groups. |
| Type        | Output                                                                                                                                                                                            |
| Format      | \$--GSV,numMsg,msgNo,numSv{,SVID,ele,az,cn0} \*CS\<CR>\<LF>                                                                                                                                       |
| Example     | \$GPGSV,3,1,10,25,68,053,47,21,59,306,49,29,56,161,49,31,36,265,49\*79<br>\$GPGSV,3,2,10,12,29,048,49,05,22,123,49,18,13,000,49,01,00,000,49\*72<br>\$GPGSV,3,3,10,14,00,000,03,16,00,000,27\*7C  |

| Field | Text               | Format            | Description                                                                                                                                                                                                                                                                                                                                     |
| ----- | ------------------ | ----------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$--GSV            |                   |                                                                                                                                                                                                                                                                                                                                                 |
| 2     | numMsg             | Characters        | The total number of statements. Each GSV statement outputs up to 4 visible satellite information, so when the system has more than 4 visible satellites, multiple GSV statements will be required.                                                                                                                                              |
| 3     | msgNo              | Number            | Current message number                                                                                                                                                                                                                                                                                                                          |
| 4     | numSv              | Numerical Value   | The total number of visible satellites                                                                                                                                                                                                                                                                                                          |
| 5     | {,SVID,ele,az,cn0} | Numerical Value   | In order: <br>1.) Satellite (SV) number <br>2.) Elevation angle, the value range is 0~90, the unit is degrees<br>3.) Azimuth angle, the value range is 0~359, the unit is degrees<br>4.) Carrier-to-noise ratio, the value range is 0~99, the unit is dB-Hz, <br>If there is no tracking to the current satellite, fill in the blank with zeros |
| 6     | signalId           | Numerical Value   | The GNSS signal ID defined by NMEA (0 represents all signals)<br>**Only valid for NMEA4.1 and above.**                                                                                                                                                                                                                                          |
| 7     | CS                 | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                                                                                                                                                                                                                    |
| 8     | \<CR>\<LF>         | Character         | Carriage Return and Line Feed                                                                                                                                                                                                                                                                                                                   |

### 1.5.5 RMC

| Information | RMC                                                                             |
| ----------- | ------------------------------------------------------------------------------- |
| Description | Recommended minimum positioning information                                     |
| Type        | Output                                                                          |
| Format      | \$--RMC,UTCtime,status,lat,uLat,lon,uLon,spd,cog,date,mv,mvE,mode\*CS\<CR>\<LF> |
| Example     | \$GPRMC,235316.000,A,2959.9925,S,12000.0090,E,0.009,75.020,020711,,,A\*45       |

| Field | Text       | Format            | Description                                                                                                                                          |
| ----- | ---------- | ----------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$--RMC    |                   |                                                                                                                                                      |
| 2     | UTCtime    | hhmmss.sss        | UTC time of the current location                                                                                                                     |
| 3     | status     | String            | Location Validity <br>V=receiver warning, invalid data<br>A=valid data                                                                               |
| 4     | lat        | ddmm.mmmm         | Latitude, the first 2 characters represent the degree, and the following characters represent the division.                                          |
| 5     | uLat       | Character         | Latitude direction: N-north, S-south                                                                                                                 |
| 6     | lon        | dddmm.mmmm        | Longitude, the first 3 characters represent the degree, and the following characters represent the score                                             |
| 7     | uLon       | Character         | Longitude direction: E-East, W-West                                                                                                                  |
| 8     | spd        | Numerical Value   | Ground speed, the unit is section.                                                                                                                   |
| 9     | cog        | Numerical Value   | Course Over Ground: Expressed in relation to True North, the unit is degrees.                                                                        |
| 10    | date       | ddmmyy            | Date (dd is the day, mm is the month, yy is the year)                                                                                                |
| 11    | mv         | Numerical Value   | Magnetic Variation: <br>Magnetic deflection angle, the unit is degree. Fixed as empty (No Magvar available)                                          |
| 12    | mvE        | Characters        | Magnetic deflection direction: E-east, W-west. Fixed as empty (No Magvar available)                                                                  |
| 13    | mode       | Characters        | Positioning mode logo (Note 1) is only valid for NMEA2.3 and above versions                                                                          |
| 14    | navStatus  | Characters        | Navigation status indicator (V indicates that the system does not output navigation status information) Only NMEA4.1 and the above version are valid |
| 15    | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                         |
| 16    | \<CR>\<LF> | Character         | Carriage Return and Line Feed                                                                                                                        |

| (Note 1) Positioning modes | Description                                                          |
| -------------------------- | -------------------------------------------------------------------- |
| A                          | Autonomous Mode                                                      |
| E                          | Dead Reckoning Mode (Only NMEA v3.00)                                |
| N                          | The data is invalid                                                  |
| D                          | DGPS Mode                                                            |
| M                          | No location, but there is an external input or saved prior location. |

### 1.5.6 VTG

| Information | VTG                                                  |
| ----------- | ---------------------------------------------------- |
| Description | Course Over Ground and Ground Speed                  |
| Type        | Output                                               |
| Format      | \$--VTG,cogt,T,cogm,M,sog,N,kph,K,mode\*CS\<CR>\<LF> |
| Example     | \$GPVTG,75.20,T,,M,0.009,N,0.017,K,A\*02             |

| Field | Text       | Format            | Description                                                                                  |
| ----- | ---------- | ----------------- | -------------------------------------------------------------------------------------------- |
| 1     | \$--VTG    |                   |                                                                                              |
| 2     | cogt       | Numerical value   | Course Over Ground: Referenced to True North, and the unit is degree.                        |
| 3     | T          | Characters        | True north indication, fixed as T                                                            |
| 4     | cogm       | Numerical value   | For the geomagnetic north direction, the unit is degrees.                                    |
| 5     | M          | Characters        | Magnetic north indication, fixed as M                                                        |
| 6     | sog        | Numerical value   | Ground speed, the unit is section.                                                           |
| 7     | N          | Characters        | Speed unit section, fixed as N                                                               |
| 8     | kph        | Numerical value   | Ground speed, the unit is kilometers per hour.                                               |
| 9     | K          | Characters        | Speed unit, km/h, fixed as K                                                                 |
| 10    | mode       | Characters        | Positioning mode logo (Note 1) is only valid for NMEA 2.3 and above versions                 |
| 11    | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*) |
| 12    | \<CR>\<LF> | Character         | Carriage Return and Line Feed                                                                |

| (Note 1) Positioning modes | Description                                                          |
| -------------------------- | -------------------------------------------------------------------- |
| A                          | Autonomous Mode                                                      |
| E                          | Dead Reckoning Mode (Only NMEA v3.00)                                |
| N                          | The data is invalid                                                  |
| D                          | DGPS Mode                                                            |
| M                          | No location, but there is an external input or saved prior location. |
### 1.5.7 ZDA

| Information | ZDA                                                    |
| ----------- | ------------------------------------------------------ |
| Description | Time and date information                              |
| Type        | Output                                                 |
| Format      | \$--ZDA,UTCtime,day,month,year,ltzh,ltzn\*CS\<CR>\<LF> |
| Example     | \$GPZDA,235316.000,02,07,2011,00,00\*51                |

| Field | Text       | Format            | Description                                                                                  |
| ----- | ---------- | ----------------- | -------------------------------------------------------------------------------------------- |
| 1     | \$--ZDA    |                   |                                                                                              |
| 2     | UTCtime    | hhmmss.sss        | UTC time of the current location                                                             |
| 3     | day        | Numerical Value   | Day, fixed two numbers, value range 01~31                                                    |
| 4     | month      | Numerical Value   | Month, two fixed numbers, the value range is 01~12                                           |
| 5     | year       | Numerical Value   | Year, fixed four-digit number                                                                |
| 6     | ltzh       | Numerical Value   | The time in this time zone is not supported, fixed as 00                                     |
| 7     | ltzn       | Numerical Value   | Minutes in this time zone, not supported, fixed as 00                                        |
| 8     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*) |
| 9     | \<CR>\<LF> | Character         | Carriage Return and Line Feed                                                                |

### 1.5.8 TXT

| Information | TXT                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Description | Product information                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| Type        | Output, output once when powering on                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| Format      | \$GPTXT,xx,yy,zz,info\*hh\<CR>\<LF>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| Example     | \$GPTXT,01,01,02,MA=CASIC\*27<br>Indicate the name of the manufacturer (CASIC)<br><br>\$GPTXT,01,01,02,IC=ATGB03+ATGR201\*71<br>Indicates the model of the chip or chipset (baseband chip model ATGB03, radio frequency chip model ATGR201)<br><br>\$GPTXT,01,01,02,SW=URANUS2,V2.2.1.0\*1D<br>Indicate the software name and version number (software name URANUS2, version number V2.2.1.0)<br><br>\$GPTXT,01,01,02,TB=2013-06-20,13:02:49\*43<br>Indicates the code compilation time (June 20, 2013, 13:02:49)<br><br>\$GPTXT,01,01,02,MO=GB\*77<br>Indicates the working mode of the receiver's startup (GB indicates the dual-mode mode of GPS+BDS)<br><br>\$GPTXT,01,01,02,CI=00000000\*7A<br>Indicates the customer number (the customer number is 00000000) |

| Field | Text       | Format            | Description                                                                                                                                         |
| ----- | ---------- | ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$GPTXT    |                   |                                                                                                                                                     |
| 2     | xx         | String            | The total number of statements of the current message is 01~99. If a message is too long, it needs to be divided into multiple messages to display. |
| 3     | yy         | Numerical value   | Sentence number 01~99                                                                                                                               |
| 4     | zz         | Numerical value   | Text identifier<br>00=error message<br>01=warning message<br>02=notification message<br>07=user information.                                        |
| 5     | info       |                   | Text message                                                                                                                                        |
| 6     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                        |
| 7     | \<CR>\<LF> | Character         | Carriage Return and Line Feed                                                                                                                       |

### 1.5.9 ANT

| Information | ANT                                                                                                                                                                                                                                                                    |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Description | Antenna status                                                                                                                                                                                                                                                         |
| Type        | Output                                                                                                                                                                                                                                                                 |
| Format      | \$GPTXT,xx,yy,zz,info\*hh\<CR>\<LF>                                                                                                                                                                                                                                    |
| Example     | \$GPTXT,01,01,01,ANTENNA OPEN\*25<br>Indicates the antenna status (open circuit)<br><br>\$GPTXT,01,01,01,ANTENNA OK\*35<br>Indicates the status of the antenna (good)<br><br>\$GPTXT,01,01,01,ANTENNA SHORT\*63<br>Indicates the status of the antenna (short circuit) |

| Field | Text       | Format            | Description                                                                                                                                         |
| ----- | ---------- | ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$GPTXT    |                   |                                                                                                                                                     |
| 2     | xx         | String            | The total number of statements of the current message is 01~99. If a message is too long, it needs to be divided into multiple messages to display. |
| 3     | yy         | Numerical value   | Sentence number 01~99, fixed as 01                                                                                                                  |
| 4     | zz         | Numerical value   | Text identifier, fixed as 01<br>01=warning message                                                                                                  |
| 5     | info       |                   | Text message<br>ANTENNA OPEN= Antenna Open Circuit<br>ANTENNA OK= Antenna Good<br>ANTENNA SHORT= Antenna Short Circuit                              |
| 6     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                        |
| 7     | \<CR>\<LF> | Character         | Carriage Return and Line Feed                                                                                                                       |

### 1.5.10 DHV

| Information | DHV                                                        |
| ----------- | ---------------------------------------------------------- |
| Description | Details of the receiver speed                              |
| Type        | Output                                                     |
| Format      | \$--DHV,UTCtime,speed3D,spdX,spdY,spdZ,gdspd\*CS\<CR>\<LF> |
| Example     | \$GNDHV,021150.000,0.03,0.006,-0.042,-0.026,0.06\*65       |

| Field | Text       | Format            | Description                                                                                  |
| ----- | ---------- | ----------------- | -------------------------------------------------------------------------------------------- |
| 1     | \$--DHV    | String            | Message ID, DHV statement header, ' -- ' is the system identification                        |
| 2     | UTCtime    | hhmmss.sss        | UTC time at the current moment                                                               |
| 3     | speed3D    | Numerical Value   | The three-dimensional speed of the receiver, the unit is m/s                                 |
| 4     | spdX       | Numerical Value   | Receiver ECEF-X axial direction speed, the unit is m/s                                       |
| 5     | spdY       | Numerical Value   | Receiver ECEF-Y axial direction speed, the unit is m/s                                       |
| 6     | spdZ       | Numerical Value   | Receiver ECEF-Z axial direction speed, the unit is m/s                                       |
| 7     | gdspd      | Numerical Value   | The speed of the horizontal ground direction of the receiver, the unit is m/s                |
| 8     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*) |
| 9     | \<CR>\<LF> | Character         | Carriage Return and Line Feed                                                                |

### 1.5.11 LPS

| Information | LPS (Only 5T Support)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| ----------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Description | Leap second information                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| Type        | Output                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| Format      | \$GPTXT,xx,yy,zz,LS=system,valid,utcLS,utcLSF,utcTOW,utcWNT,utcDN,utcWNF,utcA0,utcA1,leapDt,dateLsf,lsfExp,wnExp,wnExpNum\*hh\<CR>\<LF>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| Example     | \$GNZDA,235402.000,31,12,2016,00,00\*4E<br>(The current UTC time is December 31, 2016, 23:54:02)<br><br>\$GPTXT,01,01,02,LS=0,3,17,18,61,138,7,137,0,0,358,311216,,,\*64<br>(The leap second information of GPS is valid and used for timing. The current leap second and the leap second after the jump are not equal. From 17 seconds to 18 seconds, the leap second event occurs after 358 seconds (that is, 23:59:60 on December 31, 2016). At present, the receiver GPS system has no satellite that gives UTC parameter information abnormal alarm. At present, there is no satellite that gives an abnormal warning of GPS weeks.)<br><br>\$GPTXT,01,01,02,LS=1,1,3,4,0,61,6,61,0,0,358,311216,,,\*56<br>(The leap second information of Beidou is valid and not used for timing. The current leap second and the leap second after the jump are not equal, from 3 seconds to 4 seconds. The leap second event occurs 358 seconds later (that is, 23:59:60 on December 31, 2016). Note: The leap seconds of GPS and Beidou are different because their time starting reference points are different. At present, the receiver Beidou system does not have a satellite that gives abnormal alarm with UTC parameter information. At present, there is no satellite that gives an abnormal warning of the number of Beidou weeks.) |

| Field | Text       | Format            | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| ----- | ---------- | ----------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$GPTXT    | String            | Message ID, TXT statement header                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| 2     | xx         | Numerical Value   | The total number of statements in the current message is 01~99. If a message is too long, it needs to be divided into multiple messages and displayed, fixed as 01.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 3     | yy         | Numerical Value   | The statement number is 01~99, fixed as 01.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 4     | zz         | Numerical Value   | Text identifier. Fixed as 02.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| 5     | LS=        | String            | Leap second message identifier, fixed characters.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| 6     | system     | Character         | The system corresponding to leap second information. 0=GPS 1=BDS (Beidou)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 7     | valid      | Character         | The effective sign of leap second information.<br>When multiple satellite systems are jointly positioned, only one of the systems is used for timing (calibration 1PPS and UTC time) <br>0 = leap second information is invalid<br>1 = leap second information is valid, but the system is not used for timing<br>2 = leap second information is invalid, but the system has been used for timing<br>3 = leap The second information is valid, and the system has been used for timing.                                                                                                                                                                                                                                                                     |
| 8     | utcLS      | Numerical Value   | (Field 8-15 is the standard leap second 8 parameter, and the format is detailed in the ICD document of Beidou or GPS) <br>The current leap second is in seconds, and the positive number indicates that the satellite time is ahead of UTC time. Output when the leap second parameter is valid, otherwise it will be empty.                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 9     | utcLSF     | Numerical Value   | The forecast of leap seconds (after the leap second event), the unit is seconds, positive number table indicates that the satellite time is ahead of UTC time.<br>Output when the leap second parameter is valid, otherwise it is empty.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 10    | utcTOW     | Numerical Value   | The reference time of UTC correction parameters, in the week, is 4096 seconds. <br>Output when the leap second parameter is valid, otherwise it is empty.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 11    | utcWNT     | Numerical Value   | UTC The reference time of the modified parameter, the number of weeks, the unit is the week, and the model is 256.<br>Output when the leap second parameter is valid, otherwise it will be empty.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| 12    | utcDN      | Numerical Value   | The moment when leap seconds occur, the number of days in the week.<br>For the GPS system, the effective value range of this value is 1~7.<br>For the Beidou system, the effective value range of this value is 1~6. <br>1 means the end of Sunday, 2 means the end of Monday, and so on, and 7 represents the end of Saturday. <br>Output when the leap second parameter is valid, otherwise it is empty.                                                                                                                                                                                                                                                                                                                                                  |
| 13    | utcWNF     | Numerical Value   | The moment when leap seconds occur, the number of weeks, the unit is weeks, model 256. The leap second parameter is output when it is valid, otherwise it is empty.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 14    | utcA0      | Numerical Value   | Time error between UTC time and satellite time (proportional factor 2^-30), in seconds. <br>Output when the leap second parameter is valid, otherwise it is empty.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| 15    | utcA1      | Numerical Value   | The rate of change of time error between UTC time and satellite time (proportional factor 2^-50), in seconds/seconds. <br>Output when the leap second parameter is valid, otherwise it is empty.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| 16    | leapDt     | Numerical Value   | The time interval between the time when the leap second event occurs is from the current UTC time. The positive number indicates that the leap second event will occur in the future. <br>Output when the leap second parameter is valid and there is a leap second change (utcLs≠utcLsf), otherwise it is empty.                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| 17    | dateLsf    | ddmmyy            | The date corresponding to the leap second occurrence time of the forecast, day/month/year format. <br>The output is when the leap second parameter is valid and there is a leap second change (utcLs≠utcLsf), otherwise it is empty.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| 18    | lsfExp     | Hexadecimal Value | The current leap second correction time abnormal alarm of the satellite system. The relevant situation of the 32 satellites of the system is represented by an 8-bit hexadecimal value. From the lowest to the highest, satellites No. 1 to No. 32 are in order. <br>0=There is no abnormality in the satellite's leap second correction information. <br>1=The satellite corrects the abnormal information in leap seconds. If the leap second occurrence time in the message is not the experience time (June 30 or December 31), the receiver will give abnormal information, but the leap second will be adjusted according to the changed time. <br>Output when the leap second parameter is valid and there is an abnormality, otherwise it is empty. |
| 19    | wnExp      | Hexadecimal Value | The current satellite system time and weeks are abnormally alarmed (year-jumping alarm). The relevant situation of the 32 satellites of the system is represented by an 8-bit hexadecimal value. From the lowest to the highest, satellites 1 to 32 are in order. <br>0 = There is no abnormality in the number of weeks of the satellite, and there is no alarm. <br>1 = There is an abnormality in the number of weeks of the satellite, and the output is given when there is an abnormality in the star calendar time. Otherwise, it will be empty.                                                                                                                                                                                                     |
| 20    | wnExpNum   | Numeriacal Value  | The amplitude of the jump of the number of weeks in the satellite telegram. If the number of weeks jumps forward relative to the normal value, then the value is negative; vice sa, it is positive. The unit is the number of weeks. <br>Output when there is an abnormality in the star time. Otherwise, it will be empty.                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 21    | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 22    | \<CS>\<LF> | Characters        | Carriage return and line break                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |

### 1.5.12 UTC (Only 5T support)

| Information | UTC                                                                                                                      |
| ----------- | ------------------------------------------------------------------------------------------------------------------------ |
| Description | Receiver status, leap second correction to simplify information                                                          |
| Type        | Output                                                                                                                   |
| Format      | \$--UTC,UTCtime,lat,uLat,lon,uLon,FS,numSv,HDOP,hgt,uMsl,date,antSta,timeSrc,leapValid,dtLs,dtLsf,leapTime\*CS\<CR>\<LF> |
| Example     | \$GNUTC,235402.000,3200.00001,N,11900.00005,E,1,20,0.6,10.5,M,311216,0,0,1,17,18,1216\*3C                                |

| Field | Text       | Format            | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| ----- | ---------- | ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$--UTC    |                   |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| 2     | UTCtime    | hhmmss.sss        | UTC time of the current location                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| 3     | lat        | ddmm.mmmm         | Latitude, the first 2 characters represent the degree, and the following characters represent the division.                                                                                                                                                                                                                                                                                                                                                                             |
| 4     | uLat       | Character         | Latitude direction: N-north, S-south                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 5     | lon        | dddmm.mmmm        | Longitude, the first 3 characters represent the degree, and the following characters represent the score                                                                                                                                                                                                                                                                                                                                                                                |
| 6     | uLon       | Character         | Longitude direction: E-East, W-West                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| 7     | FS         | Numerical Value   | Indicates the current positioning quality (Note 1), and the field should not be empty.                                                                                                                                                                                                                                                                                                                                                                                                  |
| 8     | numSv      | Numerical Value   | The number of satellites used for positioning, 00~24                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 9     | HDOP       | Numerical Value   | Horizontal Accuracy Factor (HDOP)                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 10    | hgt        | Numerical Value   | Height                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 11    | uMsl       | Characters        | Height unit, meters, fixed character M                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 12    | date       | ddmmyy            | The current location date is in the format of day/month/year.                                                                                                                                                                                                                                                                                                                                                                                                                           |
| 13    | antSta     | Numerical Value   | Antenna status: <br>0=antenna open circuit<br>2=antenna normal<br>3=antenna short circuit                                                                                                                                                                                                                                                                                                                                                                                               |
| 14    | timeSrc    | Numerical Value   | Current timing source system:<br>0=GPS system <br>1=BDS system                                                                                                                                                                                                                                                                                                                                                                                                                          |
| 15    | leapValid  | Numerical Value   | Leap second correction value validity mark:<br>0 = no valid leap second value<br>1 = leap second value is valid                                                                                                                                                                                                                                                                                                                                                                         |
| 16    | utcLs      | Numerical Value   | The modified value of leap seconds at the current moment                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 17    | utcLsf     | Numerical Value   | If there is a forecast of leap seconds (utcLs ≠ utcLsf in the leap second correction information), it indicates the new leap second correction value of the forecast. After the leap second event, the value will continue to be output until the corrected information without the leap second forecast is received. If there is no forecast of leap seconds (in the received leap second correction information dtls equal to dtlsf), this field is empty                             |
| 18    | leapTime   | mmyy              | If there is a forecast of leap seconds (utcLs ≠ utcLsf in the leap seconds revision information), this field indicates the time of the forecast of leap seconds. After the leap second event occurs, the value will continue to be output until the correction information without leap second forecast is received. If there is no forecast of leap seconds (dtls is equal to dtlsf in the received leap second correction information), the field is empty. The format is month/year. |
| 19    | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                                                                                                                                                                                                                                                                                                                                                            |
| 20    | \<CS>\<LF> | Characters        | Carriage return and line break                                                                                                                                                                                                                                                                                                                                                                                                                                                          |

| Field 7 Value | Position Quality Description                                         |
| ------------- | -------------------------------------------------------------------- |
| 0             | Location is unavailable or invalid                                   |
| 1             | GPS SPS positioning mode, effective positioning                      |
| 2\*           | Differential GPS, SPS mode, fix valid                                |
| 3-5\*         | Not supported                                                        |
| 6             | Dead Reckoning Mode, fix valid. (Only valid for NMEA 2.3 and above.) |
### 1.5.13 GST

| Information | GST                                                                                     |
| ----------- | --------------------------------------------------------------------------------------- |
| Description | Detailed details of the measurement accuracy of the receiver's pseudo-distance          |
| Type        | Output                                                                                  |
| Format      | /$--GST,UTCtime,RMS,stdDevMaj,stdfDevMin,orientation,stdLat,stdLon,stdAlt\*CS\<CR>\<LF> |
| Example     | \$BDGST,081409.000,0.5,,,,0.2,0.1,0.4\*5E                                               |

| Field | Text        | Format            | Description                                                                                                                |
| ----- | ----------- | ----------------- | -------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$--GST     |                   |                                                                                                                            |
| 2     | UTCtime     | hhmmss.sss        | UTC time of the current location                                                                                           |
| 3     | RMS         | Numerical value   | During the positioning process, the RMS value of the standard deviation of the receiver's pseudo-distance error, in meters |
| 4     | stdDevMaj   | Numerical value   | The position standard deviation in the direction of the elliptical semi-long axis of the receiver is not supported.        |
| 5     | stdDevMin   | Numerical value   | The position standard deviation in the direction of the elliptical semi-short axis of the receiver is not supported.       |
| 6     | orientation | Numerical value   | The direction of the elliptical semi-long axis of the receiver is not supported.                                           |
| 7     | stdLat      | Numerical value   | The standard deviation of the receiver's latitude error, unit meter                                                        |
| 8     | stdLon      | Numerical value   | The standard deviation of the receiver's longitude error, in meters                                                        |
| 9     | stdAlt      | Numerical value   | The standard deviation of the receiver's altitude error, in meters                                                         |
| 10    | CS          | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                               |
| 11    | \<CS>\<LF>  | Characters        | Carriage return and line break                                                                                             |

### 1.5.14 INS (Only 5S series support)

| Information | INS                                                                                                                                                                                                                                                                                                                                                                                              |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Description | Inertial Navigation System (INS) Information                                                                                                                                                                                                                                                                                                                                                     |
| Type        | Output                                                                                                                                                                                                                                                                                                                                                                                           |
| Format      | \$GPTXT,xx,yy,zz,INS\_INF=sensorID,attMode,status,sesorOK,RAM,ramStart\*hh\<CR>\<LF>                                                                                                                                                                                                                                                                                                             |
| Example     | \$GPTXT,01,01,02,INS_INF=1,3,5,0,0,RAM,1\*11<br>Example:<br>K=1, current module sensor type 1<br>L=3, when installing the module package X-axis, you only need to think about the left side of the vehicle<br>m=5, the module currently outputs the RXM_SENSOR statement, and there are 5 groups of MEMS sampling data in each statement<br>n=0, the Kalman navigation filter does not converge. |

| Field | Text       | Format            | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| ----- | ---------- | ----------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$GPTXT    |                   |                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| 2     | xx         | Numerical value   | The total number of statements in the current message is 01~99. If a message is too long, it needs to be divided into multiple messages and displayed, fixed as 01.                                                                                                                                                                                                                                                                                                    |
| 3     | yy         | Numerical value   | The statement number is 01~99, fixed as 01.                                                                                                                                                                                                                                                                                                                                                                                                                            |
| 4     | zz         | Numerical value   | Text identifier.                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 5     | INS_INF    | String            | Fixed as INS_INF, which is used for INS information marking.                                                                                                                                                                                                                                                                                                                                                                                                           |
| 6     | sensorID   | Numerical value   | The sensor type used in the current module: 1 or 2                                                                                                                                                                                                                                                                                                                                                                                                                     |
| 7     | attMode    | Numerical value   | The mode configuration of the module relative to the installation posture of the vehicle can be taken from the range of values: 0, 1, 2. 3.<br>0: The module X axis points to the front of the vehicle.<br>1: The X axis of the module points to the right side of the vehicle.<br>2: The module X axis points to the rear of the vehicle.<br>3: The X axis of the module points to the left side of the vehicle.<br>9: Relative posture of adaptive estimation module |
| 8     | fs         | Numerical value   | It is used for the sampling number in the RXM SENSOR statement that is only used to output the original data of the internal MEMS. Range of values: 0, 1, _ 2, 5, 10, 25, 50.<br>If m=0, it means that the RXM SENSOR statement is not output; <br>If m! =0, indicating the output of RXM SENSOR statements per second,<br>() a statement contains m groups of MEMS sensor sampling data                                                                               |
| 9     | status     | Numerical value   | It is used to display the convergence state of the combined navigation filter. n=2 means that it has converged.                                                                                                                                                                                                                                                                                                                                                        |
| 10    | sesorOK    | Numerical value   | -                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 11    | RAM        | String            | Fixed as RAM                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| 12    | ramStart   | Numerical value   | 1: The space calculation function is turned on immediately if the backup power is turned on<br>0: The space calculation function is turned off immediately after the backup power is turned on.<br>Turn off by default                                                                                                                                                                                                                                                 |
| 13    | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                                                                                                                                                                                                                                                                                                                                           |
| 14    | \<CS>\<LF> | Characters        | Carriage return and line break                                                                                                                                                                                                                                                                                                                                                                                                                                         |

## 1.6 NMEA Custom Commands

### 1.6.1 CAS00

| Information | CAS00                                                                                                                                            |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| Description | SAVE THE CURRENT CONFIGURATION INFORMATION IN FLASH. EVEN IF THE RECEIVER IS COMPLETELY OUT OF POWER, THE INFORMATION IN FLASH WILL NOT BE LOST. |
| Type        | Input                                                                                                                                            |
| Format      | \$PCAS00\*CS\<CR>\<LF>                                                                                                                           |
| Example     | \$PCAS00\*01                                                                                                                                     |

| Field | Text       | Format            | Description                                                                                  |
| ----- | ---------- | ----------------- | -------------------------------------------------------------------------------------------- |
| 1     | \$PCAS00   | String            | Message ID, statement header 2                                                               |
| 2     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*) |
| 3     | \<CS>\<LF> | Characters        | Carriage return and line break                                                               |
### 1.6.2 CAS01

| Information | CAS01                                  |
| ----------- | -------------------------------------- |
| Description | Set the serial communication baud rate |
| Type        | Input                                  |
| Format      | \$PCAS01,br\*CS\<CR>\<LF>              |
| Example     | \$PCAS01,1\*1D                         |

| Field | Text       | Format            | Description                                                                                                 |
| ----- | ---------- | ----------------- | ----------------------------------------------------------------------------------------------------------- |
| 1     | \$PCAS01   | String            |                                                                                                             |
| 2     | br         | Number            | Baud rate configuration.<br>0=4800bps<br>1=9600bps<br>2=19200bps<br>3=38400bps<br>4=57600bps<br>5=115200bps |
| 3     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                |
| 4     | \<CS>\<LF> | Characters        | Carriage return and line break                                                                              |

### 1.6.3 CAS02

| Information | CAS02                         |
| ----------- | ----------------------------- |
| Description | Set the location update rate  |
| Type        | Input                         |
| Format      | \$PCAS02,fixInt\*CS\<CR>\<LF> |
| Example     | \$PCAS02,1000\*2E             |

| Field | Text       | Format            | Description                                                                                                                                                                                                                                                                                                                                                                                                        |
| ----- | ---------- | ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 1     | \$PCAS02   | String            |                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 2     | fixInt     | Numerical Value   | The time interval for positioning updates is ms. <br>1000 = update rate is 1Hz, output 1 positioning point per second<br>500 = update rate is 2Hz, output 2 positioning points per second<br>250 = update rate is 4Hz, output 4 positioning points per second<br>200 = update rate is 5Hz, output 5 positioning points per second<br>100=The update rate is 10Hz, and 10 positioning points are output per second. |
| 3     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                                                                                                                                                                                                                                                                                       |
| 4     | \<CS>\<LF> | Characters        | Carriage return and line break                                                                                                                                                                                                                                                                                                                                                                                     |

### 1.6.4 CAS03

| Information | CAS03                                                                                                            |
| ----------- | ---------------------------------------------------------------------------------------------------------------- |
| Description | Set the NMEA statement that requires output or stop output.                                                      |
| Type        | Input                                                                                                            |
| Format      | \$PCAS03,nGGA,nGLL,nGSA,nGSV,nRMC,nVTG,nZDA,nANT,nDHV,nLPS,res1,res2,nUTC,nGST,res3,res4,res5,nTIM\*CS\<CR>\<LF> |
| Example     | \$PCAS03,1,1,1,1,1,1,1,1,0,0,,,1,1,,,,1\*33                                                                      |

| Field | Text       | Format            | Description                                                                                                                                                                                                                                                            |
| ----- | ---------- | ----------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$PCAS03   | String            |                                                                                                                                                                                                                                                                        |
| 2     | nGGA       | Numerical value   | GGA output frequency, the output frequency of the statement is based on the positioning update rate. n (0~9) means that there is a positioning output every n times, 0 means that the statement is not output, and the empty one maintains the original configuration. |
| 3     | nGLL       | Numerical value   | GLL output frequency, the same as nGGA                                                                                                                                                                                                                                 |
| 4     | nGSA       | Numerical value   | GSA output frequency, the same as nGGA                                                                                                                                                                                                                                 |
| 5     | nGSV       | Numerical value   | GSV output frequency, the same as nGGA                                                                                                                                                                                                                                 |
| 6     | nRMC       | Numerical value   | RMC output frequency, the same as nGGA                                                                                                                                                                                                                                 |
| 7     | nVTG       | Numerical value   | VTG output frequency, the same as nGGA                                                                                                                                                                                                                                 |
| 8     | nZDA       | Numerical value   | ZDA output frequency, the same as nGGA                                                                                                                                                                                                                                 |
| 9     | nANT       | Numerical value   | ANT output frequency, the same as nGGA                                                                                                                                                                                                                                 |
| 10    | nDHV       | Numerical value   | DHV output frequency, the same as nGGA                                                                                                                                                                                                                                 |
| 11    | nLPS       | Numerical value   | LPS output frequency, the same as nGGA                                                                                                                                                                                                                                 |
| 12    | res1       | Numerical value   | Reserved                                                                                                                                                                                                                                                               |
| 13    | res2       | Numerical value   | Reserved                                                                                                                                                                                                                                                               |
| 14    | nUTC       | Numerical value   | UTC output frequency, the same as nGGA                                                                                                                                                                                                                                 |
| 15    | nGST       | Numerical value   | GST output frequency, the same as nGGA                                                                                                                                                                                                                                 |
| 16    | res3       | Numerical value   | Reserved                                                                                                                                                                                                                                                               |
| 17    | res4       | Numerical value   | Reserved                                                                                                                                                                                                                                                               |
| 18    | res5       | Numerical value   | Reserved                                                                                                                                                                                                                                                               |
| 19    | nTIM       | Numerical value   | TIM (PCAS60) output frequency, the same as nGGA                                                                                                                                                                                                                        |
| 20    | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                                                                                                                                           |
| 21    | \<CS>\<LF> | Characters        | Carriage return and line break                                                                                                                                                                                                                                         |

### 1.6.5 CAS04

| Information | CAS04                                                                                                                                |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------ |
| Description | Configure the GNSS systems                                                                                                           |
| Type        | Input                                                                                                                                |
| Format      | \$PCAS04,mode\*hh\<CR>\<LF>                                                                                                          |
| Example     | \$PCAS04,3\*1A - Beidou and GPS dual mode<br>\$PCAS04,1\*18 - single GPS working mode<br>\$PCAS04,2\*1B - single Beidou working mode |

| Field | Text       | Format            | Description                                                                                                                                                                                                         |
| ----- | ---------- | ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$PCAS04   | String            |                                                                                                                                                                                                                     |
| 2     | mode       | Number            | GNSS system configuration. For the characteristic product models, the following part configuration is supported.<br>1=GPS<br>2=BDS<br>3=GPS+BDS<br>4=GLONASS<br>5=GPS+GLONASS<br>6=BDS+GLONASS<br>7=GPS+BDS+GLONASS |
| 3     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                                                                                        |
| 4     | \<CS>\<LF> | Characters        | Carriage return and line break                                                                                                                                                                                      |

### 1.6.6 CAS05

| Information | CAS05                                                                                                                                                                                                                                            |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Description | Set the NMEA protocol type selection. There are many types of protocols for multi-mode navigation receivers, and there are also many data protocol standards. This receiver product can support a variety of protocols (optional configuration). |
| Type        | Input                                                                                                                                                                                                                                            |
| Format      | \$PCAS05,ver\*CS\<CR>\<LF>                                                                                                                                                                                                                       |
| Example     | \$PCAS05,1\*19                                                                                                                                                                                                                                   |

| Field | Text       | Format            | Description                                                                                  |
| ----- | ---------- | ----------------- | -------------------------------------------------------------------------------------------- |
| 1     | \$PCAS05   | String            |                                                                                              |
| 2     | mode       | Number            | NMEA Protocol Type Selection (Note 1)                                                        |
| 3     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*) |
| 4     | \<CS>\<LF> | Characters        | Carriage return and line break                                                               |

| (Note 1) NMEA Mode | Description                                                                                                                                                     |
| ------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 2                  | Compatible with NMEA 4.1 and above versions                                                                                                                     |
| 5                  | Compatible with the BDS/GPS dual-mode protocol of China Transportation Information Center, compatible with NMEA 2.3 and above, compatible with NMEA4.0 protocol |
| 9                  | Compatible with single GPS NMEA0183 protocol, compatible with NMEA 2.2 version                                                                                  |
### 1.6.7 CAS06

| Information | CAS06                       |
| ----------- | --------------------------- |
| Description | Query product information   |
| Type        | Input                       |
| Format      | \$PCAS06,info\*CS\<CR>\<LF> |
| Example     | \$PCAS06,0\*1B              |

| Field | Text       | Format            | Description                                                                                                                                                                                                                                                                                    |
| ----- | ---------- | ----------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$PCAS06   | String            |                                                                                                                                                                                                                                                                                                |
| 2     | info       | Number            | Query the information type of the product. The information content refers to 1.5.8. <br>0=Query firmware version number<br>1=Query hardware model and serial number <br>2=Query the working mode of multi-mode receiver<br>3=Query product customer number<br>5=Query upgrade code information |
| 3     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                                                                                                                                                                   |
| 4     | \<CS>\<LF> | Characters        | Carriage return and line break                                                                                                                                                                                                                                                                 |

### 1.6.8 CAS10

| Information | CAS10                                                                                                                        |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------- |
| Description | Receiver restart                                                                                                             |
| Type        | Input                                                                                                                        |
| Format      | \$PCAS10,rs\*CS\<CR>\<LF>                                                                                                    |
| Example     | \$PCAS10,0\*1C - Hot start<br>\$PCAS10,1\*1D - Warm start <br>\$PCAS10,2\*1E - Cold start <br>\$PCAS10,3\*1F - Factory start |

| Field | Text       | Format            | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| ----- | ---------- | ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$PCAS10   | String            |                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 2     | rs         | Number            | Startup mode configuration.<br>0= Hot start. Without using initialization information, all the data in the backup storage is valid.<br>1= Warm start. Do not use the initialization information to clear the almanac. <br>2= Cold start. Do not use initialization information to clear all data except the configuration in the backup storage.<br>3= Factory start. Clear all data in the memory and reset the receiver to the factory default configuration. |
| 3     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                                                                                                                                                                                                                                                                                                                                    |
| 4     | \<CS>\<LF> | Characters        | Carriage return and line break                                                                                                                                                                                                                                                                                                                                                                                                                                  |

### 1.6.9 CAS12

| Information | CAS12                                                                                            |
| ----------- | ------------------------------------------------------------------------------------------------ |
| Description | Receiver standby mode control<br>5L low-active module supports the command                       |
| Type        | Input                                                                                            |
| Format      | \$PCAS12,stdbysec\*CS\<CR>\<LF>                                                                  |
| Example     | \$PCAS12,60\*28<br>The receiver enters standby mode and automatically turns on after 60 seconds. |

| Field | Text       | Format            | Description                                                                                  |
| ----- | ---------- | ----------------- | -------------------------------------------------------------------------------------------- |
| 1     | \$PCAS12   | String            |                                                                                              |
| 2     | stdbysec   | Numerical value   | The time the receiver enters standby mode, up to 65535 seconds                               |
| 3     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*) |
| 4     | \<CS>\<LF> | Characters        | Carriage return and line break                                                               |

### 1.6.10 CAS20

| Information | CAS20                       |
| ----------- | --------------------------- |
| Description | Online upgrade instructions |
| Type        | Input                       |
| Format      | \$PCAS20\*CS\<CR>\<LF>      |
| Example     | \$PCAS20\*03                |

| Field | Text       | Format            | Description                                                                                  |
| ----- | ---------- | ----------------- | -------------------------------------------------------------------------------------------- |
| 1     | \$PCAS20   | String            |                                                                                              |
| 2     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*) |
| 3     | \<CS>\<LF> | Characters        | Carriage return and line break                                                               |

### 1.6.11 CAS15

| Information | CAS15                                                                                                                                                                                                                                                                                                                                          |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Description | Satellite system control instructions can be configured whether the follow-up version of any satellite<br>V5200 in the receiving system supports the command.                                                                                                                                                                                  |
| Type        | Input                                                                                                                                                                                                                                                                                                                                          |
| Format      | \$PCAS15,X,YYYYYYYY\*CS\<CR>\<LF>                                                                                                                                                                                                                                                                                                              |
| Example     | \$PCAS15,2,FFFFFF\*37 - Turn on Beidou's satellite No. 1-32 <br>\$PCAS15,2,FFFFFFE0\*42 - turn on BeiDou's satellite No. 6-32, BeiDou's satellite No. 1-5 close<br>\$PCAS15,4,FFFF\*31 - Turn on SBAS's satellite No. 1-16, that is, PRN=120-135<br>\$PCAS15,5,1F\*47 - Turn on QZSS's satellite No. 1-5, that is, PRN=193, 194, 195, 199, 197 |

| Field | Text       | Format                    | Description                                                                                                                                                                                                                                                                                                 |
| ----- | ---------- | ------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | \$PCAS15   | String                    |                                                                                                                                                                                                                                                                                                             |
| 2     | SYS_ID     | 1 number                  | 2=Beidou 1-32 satellite<br>3=Beidou 33-64 satellite<br>4=SBAS satellite (No. 1-19 SBAS satellite, corresponding to PRN 120-138)<br>5=QZSS satellite (No. 1-5 QZSS satellite, corresponding to PRN 193, 194, 195, 199, 197)                                                                                  |
| 3     | SV_MASK    | 1 to 8 hexadecimal values | Each hexadecimal character controls 4 satellites, and the rightmost controls satellites 1-4. Hexadecimal characters are converted to 4bit binary, and every 1bit corresponds to 1 satellite, 1=receive the satellite; 0=ban. <br>For example: 3FFFFFE0, indicating the prohibition of satellites 31,32,1-5. |
| 4     | CS         | Hexadecimal value         | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)                                                                                                                                                                                                                |
| 5     | \<CS>\<LF> | Characters                | Carriage return and line break                                                                                                                                                                                                                                                                              |

### 1.6.12 CAS60

| Information | CAS60                                                                                                                                               |
| ----------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| Description | Receiver time information.<br>5T-MODULE V5302 SEQUEL VERSION SUPPORTS THIS COMMAND                                                                  |
| Type        | Output                                                                                                                                              |
| Format      | \$PCAS60,UTCtime,ddmmyyyy,wn,tow,timevalid,leaps,leapsValid\*CS                                                                                     |
| Example     | \$PCAS60,091242.000,23122019,2085,119580,1,18,1\*33<br>\$PCAS60,091222.000,23122019,,,0,,0\*33<br>\$PCAS60,092011.000,23122019,2085,120029,1,,0\*33 |

| Field | Text       | Format            | Description                                                                                               |
| ----- | ---------- | ----------------- | --------------------------------------------------------------------------------------------------------- |
| 1     | \$PCAS60   | String            |                                                                                                           |
| 2     | UTCtime    | hhmmss.sss        | If the UTC time of the current moment, if leapsValid is 0, the default leaps calculation will be adopted. |
| 3     | ddmmyy     | Numerical value   | Current day, month and year                                                                               |
| 4     | wn         | Numerical value   | GPS Week Number                                                                                           |
| 5     | tow        | Numerical value   | GPS Time of Week                                                                                          |
| 6     | timeValid  | Numerical value   | Time validity (2/3/4/5 fields), 1=valid, 0=invalid                                                        |
| 7     | leaps      | Numerical value   | The gap between GPS time and UTC time, leap seconds                                                       |
| 8     | leapsValid | Numerical value   | Leap seconds leaps validity: <br>1=valid, 0=invalid                                                       |
| 9     | CS         | Hexadecimal value | Checksum, the difference or result of all characters between \$ and \* (excluding \$ and \*)              |
| 10    | \<CS>\<LF> | Characters        | Carriage return and line break                                                                            |

# 2 CASIC Standard

## 2.1 CASIC Interface
The CASIC receiver uses the custom standard interface protocol (CSIP, CASIC Standard Interface Protocol) to send data to the host, and the data is transmitted asynchronously and serially.

## 2.2 CASIC Standard Details

CSIP Data Structure

| Field 1    | Field 2                                   | Field 3       | Field 4         | Field 5            | Field 6                 |
| ---------- | ----------------------------------------- | ------------- | --------------- | ------------------ | ----------------------- |
| Header     | Payload Length                            | Message Class | Consumer Number | Effective Load (?) | Checksum                |
| 0xBA, 0xCE | No symbol short integer type 2 characters | 1 Word        | 1 Word          | <2k Characters     | 4 byte unsigned integer |
**1: Header**
Four hexadecimal characters act as the initial demarker characters (header) of the message, occupying two bytes.
**2: Payload length (len)**
The message length (two bytes) indicates the number of bytes occupied by the payload (field 5), excluding the message header, message type, message number, length and checksam fields.
**3: Messages (class)**
Occupying a byte represents the basic subset to which the current message belongs.
**4: Message number (id)**
The message class is the message number of a byte.
**5: Payload (payload)**
The payload is the specific content of the packet transmission. Its length (number of bytes) is variable and it is an integer multiple of 4.
**6: Check value (ckSum)**
The checksum is the word-by-word accumulation of all data from field 2 to field 5 (including field 2 and field 5), which occupies 4 bytes.
The calculation of the check value can follow the following algorithms:
```
ckSum = (id << 24) + (class << 16) + len;
for (i = 0; i < (len / 4); i++)
{
ckSum = ckSum + payload [i];
}
```
In the formula, payload contains all the information of field 5. During the calculation process, first assemble the parts from field 2 to field 4 (4 bytes to form a word), and then add the data of field 5 in the order of a group of 4 bytes (the first received is at a low bit).

## 2.3 CASIC Message Types
Each type of interactive message of CASIC receiver is a collection of a set of related messages.

| Name | Value | Description                                                                                    |
| ---- | ----- | ---------------------------------------------------------------------------------------------- |
| NAV  | 0x01  | Navigation Message: location, speed, time                                                      |
| TIM  | 0x02  | Timing Message: time pulse output, time mark result                                            |
| RXM  | 0x03  | Satellite measurement information output by the receiver: pseudo-distance, carrier phase, etc. |
| ACK  | 0x04  | ACK/NAK message: response message to CFG message                                               |
| CFG  | 0x05  | Configuration message: configure navigation mode, baud rate, etc.                              |
| MSG  | 0x08  | Satellite message information output by the receiver                                           |
| MON  | 0x0A  | Monitoring messages: communication status, CPU load, stack utilization, etc.                   |
| AID  | 0x0B  | Auxiliary information: almanac and other A-GPS data                                            |
## 2.4 CASIC Payload Definition
### 2.4.1 Data Structure
In order to make it more convenient to achieve structured data encapsulation, the data of the payload part is arranged in a specific way: the data in each type of consumption is closely arranged, the 2-byte value is placed on the offset address of the multiple of 2, and the 4-byte value is placed on the offset address of the multiple of 4.
### 2.4.2 Message naming
The name of the message is composed of a structure such as "message type + message name". For example: the configuration message of the configuration PPS is called: CFG-PPS.
### 2.4.3 Data type
Unless otherwise defined, the values of all multiple characters are arranged according to the small-end format. All floating-point values are transmitted according to the single-precision and double-precision standards of IEEE754.

| Symbol | Data Type                | Number of bytes | Note                 |
| ------ | ------------------------ | --------------- | -------------------- |
| U1     | Unsigned characters      | 1               |                      |
| I1     | Signed characters        | 1               | Make up the code (1) |
| U2     | Unsigned short integer   | 2               |                      |
| I2     | Signed short integer     | 2               | Make up the code (1) |
| U4     | Unsigned long integer    | 4               |                      |
| I4     | Signed long integer      | 4               | Make up the code (1) |
| R4     | IEEE754 Single Precision | 4               |                      |
| R8     | IEEE754 Double Precison  | 8               |                      |
## 2.5 CASIC Message Responses
Define the input and output mechanism of the receiver message. When the receiver receives a CFG-type message, it needs to reply to an ACK-ACK or ACK-NACK message according to whether the assigned message is processed correctly. Until the receiver replies to a received CFG message, the sender may not send a second CFG message. There is no need to reply to other messages received by the receiver.
## 2.6 CASIC Message Overview

Class NAV

| Message name | Class/ID  | Message Length | Type   | Description                                           |
| ------------ | --------- | -------------- | ------ | ----------------------------------------------------- |
| NAV-STATUS   | 0x01 0x00 | 80             | Cyclic | Receiver navigation status                            |
| NAV-DOP      | 0x01 0x01 | 28             | Cyclic | Geometric accuracy factor                             |
| NAV-SOL      | 0x01 0x02 | 72             | Cyclic | Simplified PVT navigation information                 |
| NAV-PV       | 0x01 0x03 | 80             | Cyclic | Location and speed information                        |
| NAV-TIMEUTC  | 0x01 0x10 | 24             | Cyclic | UTC time information                                  |
| NAV-CLOCK    | 0x01 0x11 | 64             | Cyclic | Clock information                                     |
| NAV-GPSINFO  | 0x01 0x20 | 8 +12\*N       | Cyclic | GPS Satellite information, N=Number of satellites     |
| NAV-BDSINFO  | 0x01 0x21 | 8 +12\*N       | Cyclic | Beidou Satellite information, N=Number of satellites  |
| NAV-GLNINFO  | 0x01 0x22 | 8 +12\*N       | Cyclic | GLONASS Satellite information, N=Number of satellites |
Class TIM

| Message name | Class/ID  | Message Length | Type   | Description             |
| ------------ | --------- | -------------- | ------ | ----------------------- |
| TIM-TP       | 0x02 0x00 | 24             | Cyclic | Timed pulse information |
Class RXM

| Message name | Class/ID  | Message Length | Type   | Description                                                           |
| ------------ | --------- | -------------- | ------ | --------------------------------------------------------------------- |
| RXM-MEASX    | 0x03 0x10 | 16+32\*N       | Cyclic | Original measurement information of pseudo-distance and carrier phase |
| RXM-SVPOS    | 0x03 0x11 | 16+48\*N       | Cyclic | Satellite location information                                        |
Class ACK

| Message name | Class/ID  | Message Length | Type     | Description                                                       |
| ------------ | --------- | -------------- | -------- | ----------------------------------------------------------------- |
| ACK-NACK     | 0x05 0x00 | 4              | Response | The reply indicates that the message was not received correctly.  |
| ACK-ACK      | 0x05 0x01 | 4              | Response | The reply indicates that the message has been received correctly. |
Class CFG

| Message name | Class/ID  | Message Length | Type          | Description                                                      |
| ------------ | --------- | -------------- | ------------- | ---------------------------------------------------------------- |
| CFG-PRT      | 0x06 0x00 | 0/8            | Query/Setting | Query/configure the working mode of UART                         |
| CFG-MSG      | 0x06 0x01 | 0/4            | Query/Setting | Query/configure the frequency of UART transmission               |
| CFG-RST      | 0x06 0x02 | 4              | Set up        | Restart the receiver/clear the saved data structure              |
| CFG-TP       | 0x06 0x03 | 0/16           | Query/Setting | Query/configure the relevant parameters of the receiver PPS      |
| CFG-RATE     | 0x06 0x04 | 0/4            | Query/Setting | Query/configure the navigation rate of the receiver              |
| CFG-CFG      | 0x06 0x05 | 4              | Set up        | Clear, save and load configuration information                   |
| CFG-TMODE    | 0x06 0x06 | 0/28           | Query/Setting | Query/configure the timing mode of the receiver PPS              |
| CFG-NAVX     | 0x06 0x07 | 0/44           | Query/Setting | Query/professional configuration of navigation engine parameters |
| CFG-GROUP    | 0x06 0x08 | 0/56           | Query/Setting | Query/configure the group delay parameters of GLONASS            |
Class MSG

| Message name | Class/ID  | Message Length | Type     | Description                               |
| ------------ | --------- | -------------- | -------- | ----------------------------------------- |
| MSG-BDSUTC   | 0x08 0x00 | 20             | Cyclical | UTC information of the BDS system.        |
| MSG-BDSION   | 0x08 0x01 | 16             | Cyclical | Ionosphere information of the BDS system. |
| MSG-BDSEPH   | 0x08 0x02 | 92             | Cyclical | Ephemeris information of the BDS system.  |
| MSG-GPSUTC   | 0x08 0x05 | 20             | Cyclical | UTC information of the GPS system.        |
| MSG-GPSION   | 0x08 0x06 | 16             | Cyclical | Ionosphere information of the GPS system  |
| MSG-GPSEPH   | 0x08 0x07 | 72             | Cyclical | GPS system ephemeris information.         |
| MSG-GLNEPH   | 0x08 0x08 | 68             | Cyclical | GLONASS system ephemeris information.     |
Class MON

| Message name | Class/ID  | Message Length | Type              | Description                                |
| ------------ | --------- | -------------- | ----------------- | ------------------------------------------ |
| MON-VER      | 0x0A 0x04 | 64             | Response to query | Output version information                 |
| MON-HW       | 0x0A 0x09 | 56             | Response to query | Various configuration statuses of hardware |
Class AID

| Message name | Class/ID  | Message Length | Type        | Description                                                           |
| ------------ | --------- | -------------- | ----------- | --------------------------------------------------------------------- |
| AID-INI      | 0x0B 0x01 | 56             | Query/Input | Auxiliary position, time, frequency, clock frequency bias information |
| AID-HUI      | 0x0B 0x03 | 60             | Input       | Auxiliary health information, UTC parameters, ionosphere parametersd  |
## 2.7 NAV (0x01)
Navigation results: location, speed, time, accuracy, heading, geometric accuracy factor and number of satellites, etc. NAV messages are divided into several types, which contain different information.

### 2.7.1 NAV-STATUS (0x01 0x00)

| Name              | NAV-STATUS                 |
| ----------------- | -------------------------- |
| Description       | Receiver Navigation Status |
| Transmission Type | Periodic / Query           |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 80             | 0x01 0x00  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name    | Units | Description                                                                    |
| ---- | --------- | ----- | ------------- | ----- | ------------------------------------------------------------------------------ |
| 0    | U4        | -     | runTime       | ms    | Running time from power-on/reset                                               |
| 4    | U2        | -     | fixInterval   | ms    | Positioning time interval                                                      |
| 6    | U1        | -     | posValid      | -     | Positioning State (Note 1)                                                     |
| 7    | U1        | -     | velValid      | -     | Velocity State (Note 2)                                                        |
| 8    | U1*32     | -     | gpsMsgFlag    | -     | The validity of the almanac and the ephemeris of 32 GPS satellites (Note 3)    |
| 40   | U1*24     | -     | glnMsgFlag    | -     | The validity of the ephemeris of 24 GLONASS satellites (Note 3)                |
| 64   | U1*14     | -     | bdsMsgFlag    | -     | The validity of the almanac and the ephemeris of 14 Beidou satellites (Note 3) |
| 78   | U1        | -     | gpsUtcionFlag | -     | Validity of UTC Time and ionosphere information for GPS (Note 4)               |
| 79   | U1        | -     | bdsUtcionFlag | -     | Validity of UTC Time and ionosphere information for Beidou (Note 4)            |
Note 1: Positioning State

| Value | State Description                |
| ----- | -------------------------------- |
| 0     | Position is Invalid              |
| 1     | External Position Input          |
| 2     | Coarse Position Estimate         |
| 3     | Use Last Known Position          |
| 4     | Position Estimation              |
| 5     | Fast mode positioning            |
| 6     | 2D Positioning                   |
| 7     | 3D Positioning                   |
| 8     | GNSS + Dead Reckoning Navigation |
Note 2: Velocity State

| Value | State Description                |
| ----- | -------------------------------- |
| 0     | Position is Invalid              |
| 1     | External Velocity Input          |
| 2     | Coarse Velocity Estimate         |
| 3     | Use Last Known Velocity          |
| 4     | Velocity Estimation              |
| 5     | Fast mode Velocity               |
| 6     | 2D Velocity                      |
| 7     | 3D Velocity                      |
| 8     | GNSS + Dead Reckoning Navigation |
Note 3: Validity of Satellite Data
The high 4 bits indicate the validity mark of the almanac, and the low 4 bits indicate the validity mark of the ephemeris.

| Value | Mode Description |
| ----- | ---------------- |
| 0     | Missing          |
| 1     | Invalid          |
| 2     | Expired          |
| 3     | Valid            |
Note 4: UTC and Ionospheric Parameter Validity
The high 4-bit text validity mark represents the text validity mark of UTC parameters, and the low 4-bit indicates the ionosphere parameter.

| Value | Mode Description |
| ----- | ---------------- |
| 0     | Missing          |
| 1     | Invalid          |
| 2     | Expired          |
| 3     | Valid            |
### 2.7.2 NAV-DOP (0x01 0x01)

| Name              | NAV-DOP                     |
| ----------------- | --------------------------- |
| Description       | Positioning accuracy factor |
| Transmission Type | Periodic / Query            |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 28             | 0x01 0x01  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                      |
| ---- | --------- | ----- | ---------- | ----- | -------------------------------- |
| 0    | U4        | -     | runTime    | ms    | Running time from power-on/reset |
| 4    | R4        | -     | pDop       | -     | Positioning DOP                  |
| 8    | R4        | -     | hDop       | -     | Horizontal DOP                   |
| 12   | R4        | -     | vDop       | -     | Vertical DOP                     |
| 16   | R4        | -     | nDop       | -     | North DOP                        |
| 20   | R4        | -     | eDop       | -     | East DOP                         |
| 24   | R4        | -     | tDop       | -     | Time DOP                         |
> [!info]
>  It seems that North and East DOP are axis translated to Earth North/East/Down (NED) from Earth Centered Earth Fixed (ECEF) that GNSS uses for satellite and receiver location. These two are likely to vary by the angle formed between ECEF and NED since NED is in relation to the surface of the Earth.

### 2.7.3 NAV-SOL (0x01 0x02)


| Name              | NAV-SOL                                                     |
| ----------------- | ----------------------------------------------------------- |
| Description       | PVT navigation information under the ECEF coordinate system |
| Transmission Type | Periodic / Query                                            |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 72             | 0x01 0x02  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units   | Description                                                         |
| ---- | --------- | ----- | ---------- | ------- | ------------------------------------------------------------------- |
| 0    | U4        | -     | runTime    | ms      | Running time from power-on/reset                                    |
| 4    | U1        | -     | posValid   | -       | Positioning Mode (Note 1)                                           |
| 5    | U1        | -     | velValid   | -       | Velocity Mode (Note 2)                                              |
| 6    | U1        | -     | timeSrc    | -       | Time source (Note 3)                                                |
| 7    | U1        | -     | system     | -       | Receiver's multi-mode reception mode mask (Note 4)                  |
| 8    | U1        | -     | numSV      | -       | The total number of satellites involved in the position calculation |
| 9    | U1        | -     | numSVGPS   | -       | Number of GPS satellites in the position calculation                |
| 10   | U1        | -     | numSVBDS   | -       | Number of Beidou satellites in the position calculation             |
| 11   | U1        | -     | numSVGLN   | -       | Number of GLONASS satellites in the position calculation            |
| 12   | U2        | -     | res        | -       | Reserved (?)                                                        |
| 14   | U2        | -     | week       | -       | GNSS Week number                                                    |
| 16   | R8        | -     | tow        | s       | GNSS Time of Week                                                   |
| 24   | R8        | -     | ecefX      | m       | X coordinates in the ECEF coordinate system                         |
| 32   | R8        | -     | ecefY      | m       | Y coordinates in the ECEF coordinate system                         |
| 40   | R8        | -     | ecefZ      | m       | Z coordinates in the ECEF coordinate system                         |
| 48   | R4        | -     | pAcc       | M^2     | The variance of the estimated accuracy error of the 3D position     |
| 52   | R4        | -     | ecefVX     | m/s     | X velocity in the ECEF coordinate system                            |
| 56   | R4        | -     | ecefVY     | m/s     | Y velocity in the ECEF coordinate system                            |
| 60   | R4        | -     | ecefVZ     | m/s     | Z velocity in the ECEF coordinate system                            |
| 64   | R4        | -     | sAcc       | (m/s)^2 | The variance of the estimation accuracy error of 3D speed           |
| 68   | R4        | -     | pDop       | -       | Position DOP                                                        |
Note 1: Positioning State

| Value | State Description                |
| ----- | -------------------------------- |
| 0     | Position is Invalid              |
| 1     | External Position Input          |
| 2     | Coarse Position Estimate         |
| 3     | Use Last Known Position          |
| 4     | Position Estimation              |
| 5     | Fast mode positioning            |
| 6     | 2D Positioning                   |
| 7     | 3D Positioning                   |
| 8     | GNSS + Dead Reckoning Navigation |
Note 2: Velocity State

| Value | State Description                |
| ----- | -------------------------------- |
| 0     | Position is Invalid              |
| 1     | External Velocity Input          |
| 2     | Coarse Velocity Estimate         |
| 3     | Use Last Known Velocity          |
| 4     | Velocity Estimation              |
| 5     | Fast mode Velocity               |
| 6     | 2D Velocity                      |
| 7     | 3D Velocity                      |
| 8     | GNSS + Dead Reckoning Navigation |
Note 3: Source of Time

| Value | Mode Description                                   |
| ----- | -------------------------------------------------- |
| 0     | GPS (Calculated from Time of Week and Week Number) |
| 1     | Beidou                                             |
| 2     | GLONASS                                            |
Note 4: Multi-mode reception mode

| Value | Mode Description                           |
| ----- | ------------------------------------------ |
| B0    | 1 = GPS Satellites Used in Positioning     |
| B1    | 1 = Beidou Satellites Used in Positioning  |
| B2    | 1 = GLONASS Satellites Used in Positioning |
### 2.7.4 NAV-PV (0x01 0x03)

| Name              | NAV-PV                                                           |
| ----------------- | ---------------------------------------------------------------- |
| Description       | Position and speed information under the earth coordinate system |
| Transmission Type | Periodic / Query                                                 |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 80             | 0x01 0x03  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units   | Description                                                                            |
| ---- | --------- | ----- | ---------- | ------- | -------------------------------------------------------------------------------------- |
| 0    | U4        | -     | runTime    | ms      | Running time from power-on/reset                                                       |
| 4    | U1        | -     | posValid   | -       | Positioning Mode (See [[#2.7.3 NAV-SOL (0x01 0x02)]] Note 1)                           |
| 5    | U1        | -     | velValid   | -       | Velocity Mode (See [[#2.7.3 NAV-SOL (0x01 0x02)]] Note 2)                              |
| 6    | U1        | -     | system     | -       | Receiver's multi-mode reception mode mask (See [[#2.7.3 NAV-SOL (0x01 0x02)]] Note 4)  |
| 7    | U1        | -     | numSV      | -       | The total number of satellites involved in the position calculation                    |
| 8    | U1        | -     | numSVGPS   | -       | Number of GPS satellites in the position calculation                                   |
| 9    | U1        | -     | numSVBDS   | -       | Number of Beidou satellites in the position calculation                                |
| 10   | U1        | -     | numSVGLN   | -       | Number of GLONASS satellites in the position calculation                               |
| 11   | U1        | -     | res        | -       | Reserved (?)                                                                           |
| 12   | R4        | -     | pDop       | -       | Position DOP                                                                           |
| 16   | R8        | -     | lon        | deg     | Longitude                                                                              |
| 24   | R8        | -     | lat        | deg     | Latitude                                                                               |
| 32   | R4        | -     | height     | m       | Earth height (with ellipsoid as a reference)                                           |
| 36   | R4        | -     | sepGeoid   | m       | Altitude abnormality (the difference between the height of the earth and the altitude) |
| 40   | R4        | -     | hAcc       | m^2     | The variance in accuracy error of the horizontal position                              |
| 44   | R4        | -     | vAcc       | m^2     | The variance in accuracy error of the vertical position                                |
| 48   | R4        | -     | velN       | m/s     | Northward velocity in the ENU coordinate system                                        |
| 52   | R4        | -     | velE       | m/s     | Eastward velocity in the ENU coordinate system                                         |
| 56   | R4        | -     | velU       | m/s     | Upward velocity in the ENU coordinate system                                           |
| 60   | R4        | -     | speed3D    | m/s     | 3D Velocity                                                                            |
| 64   | R4        | -     | speed2D    | m/s     | Ground Speed Velocity                                                                  |
| 68   | R4        | -     | heading    | deg     | True Heading                                                                           |
| 72   | R4        | -     | sAcc       | (m/s)^2 | The variance in the accuracy error of the ground velocity                              |
| 76   | R4        | -     | cAcc       | deg \^2 | The variance in the accuracy error of heading                                          |
### 2.7.5 NAV-TIMEUTC (0x01 0x10)


| Name              | NAV-TIMEUTC          |
| ----------------- | -------------------- |
| Description       | UTC time information |
| Transmission Type | Periodic / Query     |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 24             | 0x01 0x10  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                                |
| ---- | --------- | ----- | ---------- | ----- | ---------------------------------------------------------- |
| 0    | U4        | -     | runTime    | ms    | Running time from power-on/reset                           |
| 4    | R4        | 1/c^2 | tAcc       | s^2   | Time estimation accuracy                                   |
| 8    | R4        | -     | msErr      | ms    | Residual error after milliseconds                          |
| 12   | U2        | -     | ms         | ms    | The millisecond part of UTC time, the value range is 0~999 |
| 14   | U2        | -     | year       | year  | UTC Year (1999-2099)                                       |
| 16   | U1        | -     | month      | month | UTC Month (1-12)                                           |
| 17   | U1        | -     | day        | day   | UTC Day (1-31)                                             |
| 18   | U1        | -     | hour       | hour  | UTC Hour (0-23)                                            |
| 19   | U1        | -     | min        | min   | UTC Min (0-59)                                             |
| 20   | U1        | -     | sec        | s     | UTC Sec (0-59)                                             |
| 21   | U1        | -     | valid      | -     | Time Validity Mode (Note 1)                                |
| 22   | U1        | -     | timeSrc    | -     | Time Source Used (Note 2)                                  |
| 23   | U1        | -     | dateValid  | -     | Date Validity (Note 3)                                     |
Note 1: Time Validity Mode

| Value | Mode Description                                      |
| ----- | ----------------------------------------------------- |
| B0    | UTC valid flag during the week, 0=invalid, 1=valid    |
| B1    | UTC Week Number Valid, 0=invalid, 1=valid             |
| B2    | UTC leap second correction valid , 0=invalid, 1=valid |
Note 2: Time Source Used (Note 2)

| Value | Mode Description  |
| ----- | ----------------- |
| 0     | GPS Time Used     |
| 1     | Beidou Time Used  |
| 2     | GLONASS Time Used |
Note 3: Date Validity (Note 3)

| Value | Mode Description                       |
| ----- | -------------------------------------- |
| 0     | The date is invalid                    |
| 1     | Date used from external input          |
| 2     | Date used from Satellite               |
| 3     | Date received from multiple Satellites |
### 2.7.6 NAV-CLOCK (0x01 0x11)

| Name              | NAV-CLOCK                     |
| ----------------- | ----------------------------- |
| Description       | Clock calculation information |
| Transmission Type | Periodic / Query              |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 64             | 0x01 0x11  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                             |
| ---- | --------- | ----- | ---------- | ----- | --------------------------------------- |
| 0    | U4        | -     | runTime    | ms    | Running time from power-on/reset        |
| 4    | R4        | 1/c   | freqBias   | -     | Clock drift (clock frequency deviation) |
| 8    | R4        | 1/c^2 | tAcc       | s^2   | Time accuracy (variance)                |
| 12   | R4        | 1/c^2 | fAcc       | -     | Frequency accuracy (variance)           |
Time information for each Satellite Constellation (N=0 indicates GPS, 1 indicates BDS, 2 indicates GLONASS)

| Byte     | Data Type | Scale | Field Name | Units | Description                               |
| -------- | --------- | ----- | ---------- | ----- | ----------------------------------------- |
| 16+16\*N | R8        | -     | tow        | ms    | Time of Week                              |
| 24+16\*N | R4        | -     | dtUtc      | s     | Difference between satellite time and UTC |
| 28+16\*N | U2        | -     | wn         | -     | Week Number                               |
| 30+16\*N | I1        | -     | leapS      | -     | Number of UTC Leap Seconds Used           |
| 31+16\*N | U1        | -     | valid      | -     | Time validity                             |
When the repeat of this is complete, the maximum value of N is (SYSTEM_ALL-1), and the value of the current version is 2

### 2.7.7 NAV-GPSINFO (0x01 0x20)

| Name              | NAV-GPSINFO              |
| ----------------- | ------------------------ |
| Description       | GPS Satellite Infomation |
| Transmission Type | Periodic / Query         |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 8+12\*N        | 0x01 0x20  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                            |
| ---- | --------- | ----- | ---------- | ----- | ------------------------------------------------------ |
| 0    | U4        | -     | runTime    | ms    | Running time from power-on/reset                       |
| 4    | U1        | -     | numViewSv  | -     | The number of visible satellites, effective range 0-32 |
| 5    | U1        | -     | numFixSv   | -     | The number of satellites used for positioning          |
| 6    | U1        | -     | system     | -     | GNSS System Reporting (Note 1)                         |
| 7    | U1        | -     | res        | -     | Reserved (?)                                           |
Repeated part start (N=numViewSv, effective range 0~32)

| Byte     | Data Type | Scale | Field Name | Units | Description                                       |
| -------- | --------- | ----- | ---------- | ----- | ------------------------------------------------- |
| 8+12\*N  | U1        | -     | chn        | -     | GPS Channel number                                |
| 9+12\*N  | U1        | -     | svid       | -     | SV ID of the Satellite                            |
| 10+12\*N | U1        | -     | flags      | -     | Satellite status (Note 2)                         |
| 11+12\*N | U1        | -     | quality    | -     | Quality indication of signal measurement (Note 3) |
| 12+12\*N | U1        | -     | CN0        | dB-Hz | Signal-to-noise ratio                             |
| 13+12\*N | I1        | -     | elev       | deg   | Elevation of Satellite (-90 - 90)                 |
| 14+12\*N | I2        | -     | azim       | deg   | Azimuth of Satellite (0 - 360)                    |
| 16+12\*N | R4        | -     | prRes      | m     | Pseudo-range residual                             |
Note 1: GNSS System Reporting

| Value | Description |
| ----- | ----------- |
| 0     | GPS         |
| 1     | Beidou      |
| 2     | GLONASS     |
Note 2: Satellite status

| Value | Description                                                                                                                                                                           |
| ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| B0    | 1 = Satellite is in the position calculation                                                                                                                                          |
| B1-B3 | Reserved                                                                                                                                                                              |
| B4    | 1=Satellite prediction information is invalid                                                                                                                                         |
| B5    | Reserved                                                                                                                                                                              |
| B7:B6 | 00 = Reserved<br>01 = The forecast information of the satellite is based on the Almanac<br>10 = Reserved <br>11 = The forecast information of the satellite is based on the ephemeris |
Note 3: Quality indication of signal measurement

| Quality | Description                                                                                                                                                                                                  |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| B0      | = 1, indicating that the pseudo-distance measurement value *prMes* is valid                                                                                                                                  |
| B1      | = 1, indicating that the carrier phase measurement value *cpMes* is valid                                                                                                                                    |
| B2      | = 1, the display half-circumference is effective (inverted PI correction is effective)<br>(Side Note: This translation is a bit rough. This version uses the Japanese translation for clarity)               |
| B3      | = 1, indicating that the half-circumference is reduced from the measured value of the on-board phase<br>(Side Note: This translation is a bit rough. This version uses the Japanese translation for clarity) |
| B4      | Reserved                                                                                                                                                                                                     |
| B5      | = 1, indicating that the carrier frequency is valid                                                                                                                                                          |
| B6-B7   | Reserved                                                                                                                                                                                                     |
### 2.7.8 NAV-BDSINFO (0x01 0x21)

| Name              | NAV-BDSINFO                 |
| ----------------- | --------------------------- |
| Description       | Beidou Satellite Infomation |
| Transmission Type | Periodic / Query            |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 8+12\*N        | 0x01 0x21  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                                           |
| ---- | --------- | ----- | ---------- | ----- | --------------------------------------------------------------------- |
| 0    | U4        | -     | runTime    | ms    | Running time from power-on/reset                                      |
| 4    | U1        | -     | numViewSv  | -     | The number of visible satellites, effective range 0-32                |
| 5    | U1        | -     | numFixSv   | -     | The number of satellites used for positioning                         |
| 6    | U1        | -     | system     | -     | GNSS System Reporting (See [[#2.7.7 NAV-GPSINFO (0x01 0x20)]] Note 1) |
| 7    | U1        | -     | res        | -     | Reserved (?)                                                          |
Repeated part start (N=numViewSv, effective range 0~32)

| Byte     | Data Type | Scale | Field Name | Units | Description                                                                              |
| -------- | --------- | ----- | ---------- | ----- | ---------------------------------------------------------------------------------------- |
| 8+12\*N  | U1        | -     | chn        | -     | GPS Channel number                                                                       |
| 9+12\*N  | U1        | -     | svid       | -     | SV ID of the Satellite                                                                   |
| 10+12\*N | U1        | -     | flags      | -     | Satellite status (See [[#2.7.7 NAV-GPSINFO (0x01 0x20)]] Note 2)                         |
| 11+12\*N | U1        | -     | quality    | -     | Quality indication of signal measurement (See [[#2.7.7 NAV-GPSINFO (0x01 0x20)]] Note 3) |
| 12+12\*N | U1        | -     | CN0        | dB-Hz | Signal-to-noise ratio                                                                    |
| 13+12\*N | I1        | -     | elev       | deg   | Elevation of Satellite (-90 - 90)                                                        |
| 14+12\*N | I2        | -     | azim       | deg   | Azimuth of Satellite (0 - 360)                                                           |
| 16+12\*N | R4        | -     | prRes      | m     | Pseudo-range residual                                                                    |
### 2.7.9 NAV-GLNINFO (0x01 0x22)

| Name              | NAV-BDSINFO                  |
| ----------------- | ---------------------------- |
| Description       | GLONASS Satellite Infomation |
| Transmission Type | Periodic / Query             |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 8+12\*N        | 0x01 0x22  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                                           |
| ---- | --------- | ----- | ---------- | ----- | --------------------------------------------------------------------- |
| 0    | U4        | -     | runTime    | ms    | Running time from power-on/reset                                      |
| 4    | U1        | -     | numViewSv  | -     | The number of visible satellites, effective range 0-32                |
| 5    | U1        | -     | numFixSv   | -     | The number of satellites used for positioning                         |
| 6    | U1        | -     | system     | -     | GNSS System Reporting (See [[#2.7.7 NAV-GPSINFO (0x01 0x20)]] Note 1) |
| 7    | U1        | -     | res        | -     | Reserved (?)                                                          |
Repeated part start (N=numViewSv, effective range 0~32)

| Byte     | Data Type | Scale | Field Name | Units | Description                                                                              |
| -------- | --------- | ----- | ---------- | ----- | ---------------------------------------------------------------------------------------- |
| 8+12\*N  | U1        | -     | chn        | -     | GPS Channel number                                                                       |
| 9+12\*N  | U1        | -     | svid       | -     | SV ID of the Satellite                                                                   |
| 10+12\*N | U1        | -     | flags      | -     | Satellite status (See [[#2.7.7 NAV-GPSINFO (0x01 0x20)]] Note 2)                         |
| 11+12\*N | U1        | -     | quality    | -     | Quality indication of signal measurement (See [[#2.7.7 NAV-GPSINFO (0x01 0x20)]] Note 3) |
| 12+12\*N | U1        | -     | CN0        | dB-Hz | Signal-to-noise ratio                                                                    |
| 13+12\*N | I1        | -     | elev       | deg   | Elevation of Satellite (-90 - 90)                                                        |
| 14+12\*N | I2        | -     | azim       | deg   | Azimuth of Satellite (0 - 360)                                                           |
| 16+12\*N | R4        | -     | prRes      | m     | Pseudo-range residual                                                                    |
### NAV-IMUATT (0x01 0x06)

| Name              | NAV-IMUATT                                                                                         |
| ----------------- | -------------------------------------------------------------------------------------------------- |
| Description       | The attitude of the IMU coordinate system relative to the local navigation coordinate system (NED) |
| Transmission Type | Periodic / Query                                                                                   |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 32             | 0x01 0x06  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                        |
| ---- | --------- | ----- | ---------- | ----- | ---------------------------------- |
| 0    | U4        | -     | tow        | s     | Receiver GPS Time of Week (Note 1) |
| 4    | U2        | -     | weekNum    | -     | Receiver GPS week number (Note 1)  |
| 6    | U1        | -     | flag       | -     | Operational Flags (Note 2)         |
| 7    | U1        | -     | res        | -     | Reserved                           |
| 8    | I4        | 1e-5  | roll       | deg   | Roll angle                         |
| 12   | I4        | 1e-5  | pitch      | deg   | Pitch angle                        |
| 16   | I4        | 1e-5  | heading    | deg   | Heading angle                      |
| 20   | U4        | 1e-5  | rollAcc    | deg   | Roll Angle Accuracy                |
| 24   | U4        | 1e-5  | pitchAcc   | deg   | Pitch Angle Accuracy               |
| 28   | U4        | 1e-5  | headingAcc | deg   | Heading Angle Accuracy             |
Note 1: Receiver Time during the week

| rcvTow/wn | Refer to the meaning of rcvTow/wn in RXM-MEASX. |
| --------- | ----------------------------------------------- |
Note 2: Operational Flags

| Flag | 0x01-posture estimation is valid; 0xff posture estimation is invalid. |
| ---- | --------------------------------------------------------------------- |
### 2.8.1 TIM-TP (0x02 0x00)

| Name              | TIM-TP                  |
| ----------------- | ----------------------- |
| Description       | Timed pulse information |
| Transmission Type | Periodic / Query        |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 24             | 0x02 0x00  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                                        |
| ---- | --------- | ----- | ---------- | ----- | ------------------------------------------------------------------ |
| 0    | U4        | -     | runTime    | ms    | Running time from power-on/reset                                   |
| 4    | R4        | -     | qErr       | s     | The time quantification error corresponding to the next time pulse |
| 8    | R8        | -     | tow        | s     | The Time of Wee corresponding to the next time pulse               |
| 16   | U2        | -     | wn         | -     | The Week Number corresponding to the next time pulse               |
| 18   | U1        | -     | refTime    | -     | Reference time (Note 1)                                            |
| 19   | U1        | -     | utcValid   | -     | UTC Validity (Note 2)                                              |
| 20   | U4        | -     | res        | -     | Reserved                                                           |

Note 1: Reference Time State

| Value | Description                                                                               |
| ----- | ----------------------------------------------------------------------------------------- |
| B3:B0 | 0: GPS Time Source<br>1: Beidou Time Source<br>2: GLONASS Time Source                     |
| B7:B4 | 0: The time benchmark is UTC<br>1: The time benchmark is the indicated GNSS Constellation |
Note 2: UTC Validity Flags

| Value | Description |
| ----- | ----------- |
| 0     | Missing     |
| 1     | Reserved    |
| 2     | Expired     |
| 3     | Valid       |

### 2.9.1 RXM-MEASX (0x03 0x10)

| Name              | RXM-MEASX                                                             |
| ----------------- | --------------------------------------------------------------------- |
| Description       | Original measurement information of pseudo-distance and carrier phase |
| Transmission Type | Periodic / Query                                                      |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 16+32\*N       | 0x03 0x10  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                         |
| ---- | --------- | ----- | ---------- | ----- | --------------------------------------------------- |
| 0    | R8        | -     | rcvTow     | s     | Receiver Time of Week (Note 1)                      |
| 8    | I2        | -     | wn         | week  | GPS Week Number                                     |
| 10   | I1        | -     | leapS      | s     | UTC leap second value (Note 2)                      |
| 11   | U1        | -     | numMeas    | -     | The number of measured values, effective range 0~32 |
| 12   | U1        | -     | recStat    | -     | Receiver Status (Note 3)                            |
| 13   | U1        | -     | res1       | -     |                                                     |
| 14   | U1        | -     | res2       | -     |                                                     |
| 15   | U1        | -     | res3       | -     |                                                     |
Repeated part start (N=numMeas, effective range 0~32)

| Byte     | Data Type | Scale | Field Name | Units  | Description                                                                                                                           |
| -------- | --------- | ----- | ---------- | ------ | ------------------------------------------------------------------------------------------------------------------------------------- |
| 16+32\*N | R8        | -     | prMes      | m      | Pseudo-range Measurement Value <br>For interfrequency deviation of GLONASS, Compensate through the built-in correction table.         |
| 24+32\*N | R8        | -     | cpMes      | cycles | Carrier phase measurement value (unit: cycles) (Note 4)                                                                               |
| 32+32\*N | R4        | -     | doMes      | Hz     | Doppler measurement value (unit: Hz)<br>Closer to the satellite means Doppler is positive.                                            |
| 36+32\*N | U1        | -     | gnssid     | -      | System type.<br>0 = GPS<br>1 = Beidou<br>2 = GLONASS                                                                                  |
| 37+32\*N | U1        | -     | svid       | -      | Satellite Vehicle ID                                                                                                                  |
| 38+32\*N | U1        | -     | res4       | -      | Reserved                                                                                                                              |
| 39+32\*N | U1        | -     | reqid      | -      | The frequency number (shift quantity 8) is only valid for GLONASS. Effective value range \[1,14\], corresponding frequency \[-7,+6\]. |
| 40+32\*N | U2        | -     | locktime   | ms     | Carrier phase locking time, up to 65535ms                                                                                             |
| 42+32\*N | U1        | -     | cn0        | dB-Hz  | Carrier-to-Noise ratio                                                                                                                |
| 43+32\*N | U1        | -     | res5       | -      | Reserved                                                                                                                              |
| 44+32\*N | U1        | -     | res6       | -      | Reserved                                                                                                                              |
| 45+32\*N | U1        | -     | res7       | -      | Reserved                                                                                                                              |
| 46+32\*N | U1        | -     | trkStat    | -      | Tracking Status (Note 5)                                                                                                              |
| 47+32\*N | U1        | -     | res8       | -      | Reserved                                                                                                                              |
Note 1: Receiver Time of Week

| rcvTow | The receiver time should be as aligned with the GPS time system as much as possible. Using the receiver's weekly time *rcvTow*, the receiver's week, and leap second value *leapS* can be converted to other time systems. For more information about the different time system, please refer to the RINEX3 document. When the receiver is working in a single GLONASS mode, the UTC time can be obtained directly by subtracting the leap second value *leapS* from the receiver time without considering whether the flag bit in *recStat* is valid. |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
Note 2: UTC Leap Second

| leapS | The leap second value between GPS time and UTC time is the latest value that the receiver can know. The logo bit in *recStat* indicates whether the value is valid. |
| ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
Note 3: Receiver status

| recStat | Definition                                                                                         |
| ------- | -------------------------------------------------------------------------------------------------- |
| B0      | = 1, indicating that the leap second value *leapS* is valid (UTC correction parameter is valid).   |
| B1      | = 1, indicating that the clock rest occurs, and the receiver time jumps by an integer millisecond. |
Note 4: Carrier phase measurement value

| cpMeas | Use an approximate value to initialize the initial cycle blur of the carrier phase, so that the carrier phase measurement value is close to the pseudo-distance measurement value. The clock reset mechanism acts on both the pseudo-distance measurement value and the carrier phase measurement value, which is in line with the provisions of RINEX3. |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
Note 5: Tracking Status

| Value | Description                                                                                         |
| ----- | --------------------------------------------------------------------------------------------------- |
| B0    | = 1, indicating that the pseudo-distance measurement value *prMes* is valid                         |
| B1    | = 1, indicating that the carrier phase measurement value *cpMes* is valid                           |
| B2    | = 1, the display half-circumference is effective (inverted PI correction is effective)              |
| B3    | = 1, indicating that the half-circumference is subtracted from the on-board phase measurement value |
### 2.9.2 RXM-SVPOS (0x03 0x11)

| Name              | RXM-SVPOS                      |
| ----------------- | ------------------------------ |
| Description       | Satellite Position information |
| Transmission Type | Periodic / Query               |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 16+48\*N       | 0x03 0x11  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                         |
| ---- | --------- | ----- | ---------- | ----- | --------------------------------------------------- |
| 0    | R8        | -     | rcvTow     | s     | Receiver Time of Week (Note 1)                      |
| 8    | I2        | -     | wn         | week  | GPS Week Number (Note 1)                            |
| 10   | I1        | -     | numMeas    | -     | The number of measured values, effective range 0~32 |
| 11   | U1        | -     | res1       | -     |                                                     |
| 12   | U1        | -     | res2       | -     |                                                     |
Repeated part start (N=numMeas, effective range 0~32)

| Byte     | Data Type | Scale | Field Name | Units | Description                                         |
| -------- | --------- | ----- | ---------- | ----- | --------------------------------------------------- |
| 16+48\*N | R8        | -     | x          | m     | Satellite X coordinate in ECEF                      |
| 24+48\*N | R8        | -     | y          | m     | Satellite Y coordinate in ECEF                      |
| 32+48\*N | R8        | -     | z          | m     | Satellite Z coordinate in ECEF                      |
| 40+48\*N | R4        | -     | svdt       | m     | Satellite clock difference                          |
| 44+48\*N | R4        | -     | svdf       | m/s   | Satellite frequency deviation                       |
| 48+48\*N | R4        | -     | tropDelay  | m     | Tropospheric Delay                                  |
| 52+48\*N | R4        | -     | ionoDelay  | m     | Ionospheric Delay                                   |
| 56+48\*N | U1        | -     | svid       | -     | Satellite Vehicle ID                                |
| 57+48\*N | U1        | -     | glnFreqid  | -     | Frequency sign (offset 8), for GLONASS              |
| 58+48\*N | U1        | -     | gnssid     | -     | System type<br>0 = GPS<br>1 = Beidou<br>2 = GLONASS |
| 59+48\*N | U1        | -     | res3       | -     | Reserved                                            |
| 60+48\*N | U4        | -     | res4       | -     | Reserved                                            |

Note 1: Receiver Time of Week

| rcvTow/wn | Refer to the meaning of rcvTow/wn in RXM-MEASX. |
| --------- | ----------------------------------------------- |
### 2.9.3 RXM-SENSOR (0x03 0x07)

| Name              | RXM-SENSOR                  |
| ----------------- | --------------------------- |
| Description       | Inertial sensor information |
| Transmission Type | Periodic / Query            |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 16+16\*N       | 0x03 0x07  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                                  |
| ---- | --------- | ----- | ---------- | ----- | ------------------------------------------------------------ |
| 0    | R8        | -     | rcvTow     | s     | Receiver Time of Week (Note 1)                               |
| 8    | I2        | -     | wn         | week  | GPS Week Number (Note 1)                                     |
| 10   | I1        | -     | leapS      | s     | UTC leap second value                                        |
| 11   | U1        | -     | numMeas    | -     | The number of measured values, effective range 0~32 (Note 2) |
| 12   | U1        | -     | recStat    | -     | Receiver Status                                              |
| 13   | U1        | -     | timeSrc    | -     | 0 = GPS<br>1 = Beidou                                        |
| 14   | U1        | -     | rcvrId     | -     | 0                                                            |
| 15   | U1        | -     | res        | -     | Reserved                                                     |
Repeated part start (N=numMeas, effective range 0~32)

| Byte     | Data Type | Scale     | Field Name | Units | Description                                  |
| -------- | --------- | --------- | ---------- | ----- | -------------------------------------------- |
| 16+16\*N | I2        | 1g/16384  | accX       | m/s/s | Accelerometer X-axis measured value (Note 3) |
| 18+16\*N | I2        | 1g/16384  | accY       | m/s/s | Accelerometer X-axis measured value (Note 3) |
| 20+16\*N | I2        | 1g/16384  | accZ       | m/s/s | Accelerometer X-axis measured value (Note 3) |
| 22+16\*N | I2        | 250/32768 | gyroX      | deg/s | Gyroscope X-axis measured value (Note 4)     |
| 24+16\*N | I2        | 250/32768 | gyroY      | deg/s | Gyroscope X-axis measured value (Note 4)     |
| 26+16\*N | I2        | 250/32768 | gyroZ      | deg/s | Gyroscope X-axis measured value (Note 4)     |
| 28+16\*N | I2        | 1/326.8   | temp       | deg C | Thermometer measurement                      |
| 30+16\*N | I2        | -         | res        | -     | Reserved                                     |

Note 1: Receiver Time of Week / Week Number

| rcvTow/wn | Refer to the meaning of rcvTow/wn in RXM-MEASX. |
| --------- | ----------------------------------------------- |
Note 2: numMeas

| numMeas | Configured by CFG-MSG statement, numMeas and CFG-MSG in rate. CFG-MSG setting *rate = 0*, RXM _ SENSOR statement does not output; rate is equal to 1/2/5/10/25/50 one of several discrete values. In each statement, there is numMeas = rate per MEMS sampling data. If numMeas = 50, RXM-SENSOR  statement is output, it will be output once per second. |
| ------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |

> [!info]
> I know that this translation is super rough. We are likely not going to need this bitfield translated out since we do not have an inertial sensor that we are using.

Note 3: Accelerometer Range

| acc | The accelerometer range is -2g~+2g. |
| --- | ----------------------------------- |

Note 4: Gyroscope Range

| gyro | The measuring range of the gyroscope is -250deg/s~+250deg/s. |
| ---- | ------------------------------------------------------------ |
## 2.10 ACK (0x05)
ACK and NACK are used to reply to received CFG messages.
### 2.10.1 ACK-NACK (0x05 0x00)

| Name              | ACK-NACK                |
| ----------------- | ----------------------- |
| Description       | Not Acknoledged Message |
| Transmission Type | Response                |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 4              | 0x05 0x00  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                                       |
| ---- | --------- | ----- | ---------- | ----- | ----------------------------------------------------------------- |
| 0    | U1        | -     | clsID      | -     | Types of information that have not been received correctly        |
| 1    | U1        | -     | msgID      | -     | The Message Number that did not receive the information correctly |
| 2    | U2        | -     | res        | -     | Reserved                                                          |
### 2.10.2 ACK-ACK (0x05 0x01)

| Name              | ACK-ACK              |
| ----------------- | -------------------- |
| Description       | Acknowledged Message |
| Transmission Type | Response             |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 4              | 0x05 0x01  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                           |
| ---- | --------- | ----- | ---------- | ----- | ----------------------------------------------------- |
| 0    | U1        | -     | clsID      | -     | Types of information that that was received correctly |
| 1    | U1        | -     | msgID      | -     | The Message Number that was received correctly        |
| 2    | U2        | -     | res        | -     | Reserved                                              |
## 2.11 CFG (0x06)
Configuration information, such as setting dynamic mode, baud rate, etc. When the effective length is 0, it represents a query of the configuration information, and the system will output the data with the same symbol.
### 2.11.1 CFG-PRT (0x06 0x00)
Query information

| Name              | CFG-PRT                                                                                                                     |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------- |
| Description       | Query the working mode of UART, including two statements UART0 and UART1, and the last output of the current UART statement |
| Transmission Type | Query                                                                                                                       |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 0              | 0x06 0x00  | See Table Below | 4 bytes  |

Set information

| Name              | CFG-PRT                      |
| ----------------- | ---------------------------- |
| Description       | Set the working mode of UART |
| Transmission Type | Set                          |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 8              | 0x06 0x00  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                                                                                                                             |
| ---- | --------- | ----- | ---------- | ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0    | U1        | -     | portID     | -     | Port identifiers (0 and 1 correspond to UART0 and UART1, 0xFF represents the UART of the current connection)                                            |
| 1    | U1        | -     | protoMask  | -     | Protocol control mask, each port can support several protocols at the same time. When the corresponding bit is equal to 1, enable the protocol (Note 1) |
| 2    | U2        | -     | mode       | -     | The bit mask of UART working mode (Note 2)                                                                                                              |
| 4    | U4        | -     | baudRate   | bps   | Baud rate Setting                                                                                                                                       |
Note 1: Protocol control mask

| Value | Description                |
| ----- | -------------------------- |
| B0    | 1 = binary protocol input  |
| B1    | 1 = Text protocol input    |
| B4    | 1 = binary protocol output |
| B5    | 1 = Text protocol output   |
Note 2: UART working mode bit mask

| Bits      | Value | Description   |
| --------- | ----- | ------------- |
| \[7:6\]   | 00    | 5 bits        |
|           | 01    | 6 bits        |
|           | 10    | 7 bits        |
|           | 11    | 8 bits        |
| \[11:9\]  | 10x   | No Parity     |
|           | 001   | Odd Parity    |
|           | 000   | Even Parity   |
|           | x1x   | Reserved      |
| \[13:12\] | 00    | A stop bit    |
|           | 01    | 1.5 stop bits |
|           | 10    | Two stop bits |
|           | 11    | Reserved      |
### 2.11.2 CFG-MSG (0x06 0x01)
Query information

| Name              | CFG-MSG                                        |
| ----------------- | ---------------------------------------------- |
| Description       | Query the frequency of sending all information |
| Transmission Type | Query                                          |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 0              | 0x06 0x01  | See Table Below | 4 bytes  |

Set information

| Name              | CFG-MSG                                  |
| ----------------- | ---------------------------------------- |
| Description       | Set the frequency of information sending |
| Transmission Type | Set                                      |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 4              | 0x06 0x01  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                     |
| ---- | --------- | ----- | ---------- | ----- | ------------------------------- |
| 0    | U1        | -     | clsID      | -     | Types of information            |
| 1    | U1        | -     | msgID      | -     | Message Number                  |
| 2    | U2        | -     | rate       | -     | Transmission frequency (Note 1) |

Note 1: Frequency of information transmission

| Value  | Description                                                                                                                                                                                          |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0      | No Output                                                                                                                                                                                            |
| 1      | Output once with each position                                                                                                                                                                       |
| 2      | Output twice with each position                                                                                                                                                                      |
| N      | Output *N* times with each position<br>In particular, when *clsID=0x03, msgID=0x07*, rate represents the number of samples per second output by the sensor in the configured RXM-SENSOR information. |
| 0xFFFF | Immediately output once, and only once, which is equivalent to query output.                                                                                                                         |
> [!info]
> This is **NOT** Navigation data rate. For Nav Data Rate, see [[#2.11.5 CFG-RATE (0x06 0x04)]] This is something else that might drive the inertial data. It is best to query this and figure out what the defaults are first.

### 2.11.3 CFG-RST (0x06 0x02)
Set information

| Name              | CFG-RST                                             |
| ----------------- | --------------------------------------------------- |
| Description       | Restart the receiver/clear the saved data structure |
| Transmission Type | Setup                                               |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 4              | 0x06 0x02  | See Table Below | 4 bytes  |
Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                                                                                                    |
| ---- | --------- | ----- | ---------- | ----- | ------------------------------------------------------------------------------------------------------------------------------ |
| 0    | U2        | -     | navBbrMask | -     | Clear the battery-powered RAM. If a digit of the mask is set to 1, then the data indicated on the bit will be cleared (Note 1) |
| 2    | U1        | -     | resetMode  | -     | Reset Mode (Note 2)                                                                                                            |
| 3    | U1        | -     | startMode  | -     | Start Mode (Note 3)                                                                                                            |

Note 1: Clearing Memory

| Value | Description                             |
| ----- | --------------------------------------- |
| B0    | Ephemeris                               |
| B1    | Almanac                                 |
| B2    | Health Information                      |
| B3    | Ionosphere parameters                   |
| B4    | Receiver location information           |
| B5    | Clock drift (clock frequency deviation) |
| B6    | Crystal clock parameters                |
| B7    | UTC correction parameters               |
| B8    | RTC                                     |
| B9    | Configuration information               |
Note 2: Reset Method

| Value | Description                                           |
| ----- | ----------------------------------------------------- |
| 0     | Immediate Hardware Rest (Done through watchdog)       |
| 1     | Controlled software reset                             |
| 2     | Controlled software reset (GPS only)                  |
| 4     | Hardware reset after shutdown (Done through watchdog) |
Note 3: Start-up method

| Value | Description   |
| ----- | ------------- |
| 0     | Warm start    |
| 1     | Hot start     |
| 2     | Cold start    |
| 3     | Factory Start |
### 2.11.4 CFG-TP (0x06 0x03)
Query information

| Name              | CFG-TP                      |
| ----------------- | --------------------------- |
| Description       | Query time pulse parameters |
| Transmission Type | Query                       |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 0              | 0x06 0x03  | See Table Below | 4 bytes  |
Set information

| Name              | CFG-TP                         |
| ----------------- | ------------------------------ |
| Description       | Read/set time pulse parameters |
| Transmission Type | Setup                          |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 16             | 0x06 0x03  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                    |
| ---- | --------- | ----- | ---------- | ----- | ---------------------------------------------- |
| 0    | U4        | -     | interval   | us    | The time interval between pulses (pulse cycle) |
| 4    | U4        | -     | width      | us    | Pulse width                                    |
| 8    | U1        | -     | enable     | -     | Enable Bit (Note 1)                            |
| 9    | U1        | -     | polar      | -     | Pulse polarity configuration (Note 2)          |
| 10   | U1        | -     | timeRef    | -     | Reference time (Note 3)                        |
| 11   | U1        | -     | timeSource | -     | Time source (Note 4)                           |
| 12   | R4        | -     | userDelay  | s     | User time delay                                |

Note 1: Pulse enable sign

| Value | Description                                                                                                                           |
| ----- | ------------------------------------------------------------------------------------------------------------------------------------- |
| 0     | Turn off the pulse                                                                                                                    |
| 1     | Enable pulse                                                                                                                          |
| 2     | Pulse enables and continues to output. When it cannot be positioned normally, the pulse update rate will be automatically maintained. |
| 3     | IOutput pulses when it is normally positioned. When the receiver cannot be positioned normally, it does not output pulses.            |

Note 2: Pulse polarity configuration

| Value | Description                                           |
| ----- | ----------------------------------------------------- |
| 0     | Rising edge                                           |
| 1     | Falling edge                                          |

Note 3: Reference time

| Value | Description    |
| ----- | -------------- |
| 0     | UTC time       |
| 1     | Satellite time |
Note 4: Satellite time source

| Value | Description                                                                                           |
| ----- | ----------------------------------------------------------------------------------------------------- |
| 0     | Compulsory single GPS timing                                                                          |
| 1     | Compulsory single BDS timing                                                                          |
| 2     | Compulsory single GLONASS timing                                                                      |
| 3     | Reserved                                                                                              |
| 4     | The main use of BDS can be automatically switched to other timing systems when BDS is not available.  |
| 5     | The main GPS can be automatically switched to other timing systems when GPS is not available.         |
| 6     | The main GLONASS can be automatically switched to other timing systems when GLONASS is not available. |
| 7     | Reserved                                                                                              |
| Other | Automatically select the timing system                                                                |
### 2.11.5 CFG-RATE (0x06 0x04)
Query information

| Name              | CFG-RATE                                                                                                                                                                                                                                |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Description       | Query the time interval of positioning                                                                                                                                                                                                  |
| Transmission Type | Query                                                                                                                                                                                                                                   |
| Note              | The receiver supports different navigation rates (the default rate is updated once per second). The navigation speed will directly affect the power consumption. The faster the speed, the greater the burden on CPU and communication. |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 0              | 0x06 0x04  | See Table Below | 4 bytes  |
Set information

| Name              | CFG-RATE                                                                                                                                                                                                                                |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Description       | Set the positioning time interval                                                                                                                                                                                                       |
| Transmission Type | Setup                                                                                                                                                                                                                                   |
| Note              | The receiver supports different navigation rates (the default rate is updated once per second). The navigation speed will directly affect the power consumption. The faster the speed, the greater the burden on CPU and communication. |
Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 4              | 0x06 0x04  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                    |
| ---- | --------- | ----- | ---------- | ----- | ---------------------------------------------- |
| 0    | U2        | -     | interval   | ms    | The time interval between the two positionings |
| 2    | U2        | -     | res        | -     | Reserved                                       |
### 2.11.6 CFG-CFG (0x06 0x05)
Set information

| Name              | CFG-CFG                                        |
| ----------------- | ---------------------------------------------- |
| Description       | Clear, save and load configuration information |
| Transmission Type | Set                                            |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 4              | 0x06 0x05  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                           |
| ---- | --------- | ----- | ---------- | ----- | ----------------------------------------------------- |
| 0    | U2        | -     | mask       | -     | The mask of configuration information (Note 1)        |
| 2    | U1        | -     | mode       | -     | Operation mode for configuration information (Note 2) |
| 3    | U1        | -     | res        | -     | Reserved                                              |
Note 1: Configure the information mask

| Value | Description                                    |
| ----- | ---------------------------------------------- |
| B0    | IO port configuration information (CFG-PRT)    |
| B1    | Message configuration (CFG-MSG)                |
| B2    | INF message configuration (CFG-INF)            |
| B3    | Navigation configuration (CFG-RATE, CFG-TMODE） |
| B4    | Time pulse configuration (CFG-TP)              |
| B5    | Group delay (CFG-GROUP)                        |
Note 2: Operation mode

| Value | Description                                                          |
| ----- | -------------------------------------------------------------------- |
| 0     | Clear the permanent configuration                                    |
| 1     | Save the current configuration to the permanent configuration        |
| 2     | The permanent configuration is loaded into the current configuration |
### 2.11.7 CFG-TMODE (0x06 0x06)
Query information

| Name              | CFG-TMODE             |
| ----------------- | --------------------- |
| Description       | Query the timing mode |
| Transmission Type | Query                 |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 0              | 0x06 0x06  | See Table Below | 4 bytes  |
Set information

| Name              | CFG-TMODE                |
| ----------------- | ------------------------ |
| Description       | Read/set the timing mode |
| Transmission Type | Setup                    |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 40             | 0x06 0x06  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name   | Units | Description                                                      |
| ---- | --------- | ----- | ------------ | ----- | ---------------------------------------------------------------- |
| 0    | U4        | -     | mode         | -     | Timed mode (Note 1)                                              |
| 4    | R8        | -     | fixedPosX    | m     | X coordinates in the ECEF coordinate system                      |
| 12   | R8        | -     | fixedPosY    | m     | Y coordinates in the ECEF coordinate system                      |
| 20   | R8        | -     | fixedPosZ    | m     | Z coordinates in the ECEF coordinate system                      |
| 28   | R4        | -     | fixedPosVar  | m^2   | 3D variance of position                                          |
| 32   | U4        | -     | svinMinDur   | s     | When the timing mode is 1, the minimum measurement time interval |
| 36   | R4        | -     | svinVarLimit | m^2   | When the timing mode is 1, the positioning error limit           |
Note 1: Timed mode

| Value | Description                                                                                                                                                                                                                                                                           |
| ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0     | Self-positioning and timing at the same time                                                                                                                                                                                                                                          |
| 1     | After self-positioning for a period of time to obtain a user position with sufficient accuracy, only all available satellites are used to calculate the user's clock parameters for timing. In this mode, when the user's position is fixed, single-satellite timing can be realized. |
| 2     | The user enters the current position and only uses all available satellites to calculate the user's clock parameters for timing. In this mode, single-satellite timing can be realized.                                                                                               |
### 2.11.8 CFG-NAVX (0x06 0x07)
Query information

| Name              | CFG-NAVX                                         |
| ----------------- | ------------------------------------------------ |
| Description       | Query the configuration of the navigation engine |
| Transmission Type | Query                                            |
| Note              | Query navigation-related parameters              |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 0              | 0x06 0x07  | See Table Below | 4 bytes  |
Set information

| Name              | CFG-NAVX                                        |
| ----------------- | ----------------------------------------------- |
| Description       | Professional configuration of navigation engine |
| Transmission Type | Setup                                           |
| Note              | Configure navigation-related parameters         |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 44             | 0x06 0x07  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name   | Units | Description                                                                                                   |
| ---- | --------- | ----- | ------------ | ----- | ------------------------------------------------------------------------------------------------------------- |
| 0    | U4        | -     | mask         | -     | Parameter mask, only the corresponding bit mask is set to 1, and the parameter settings are applied. (Note 1) |
| 4    | U1        | -     | dyModel      | -     | Dynamic mode (Note 2)                                                                                         |
| 5    | U1        | -     | fixMode      | -     | Positioning mode (Note 3)                                                                                     |
| 6    | U1        | -     | minSVs       | -     | The minimum number of satellites for positioning                                                              |
| 7    | U1        | -     | maxSVs       | -     | The maximum number of satellites for positioning                                                              |
| 8    | U1        | -     | minCNO       | dB-Hz | The minimum satellite signal carrier-to-noise ratio for positioning                                           |
| 9    | U1        | -     | res1         | -     | Reserved                                                                                                      |
| 10   | U1        | -     | iniFix3D     | -     | Initialization positioning must be a 3D positioning lock (0/1)                                                |
| 11   | I1        | -     | minElev      | deg   | The minimum elevation angle of the GNSS satellite for positioning                                             |
| 12   | U1        | -     | drLimit      | s     | The maximum Dead Reckoning time without satellite signals                                                     |
| 13   | U1        | -     | navSystem    | -     | Navigation system enable logo (Note 4)                                                                        |
| 14   | U2        | -     | wnRollOver   | -     | GPS Number of Week Rollovers                                                                                  |
| 16   | R4        | -     | fixedAlt     | m     | Fixed height during 2D positioning                                                                            |
| 20   | R4        | -     | fixedAltVar  | m^2   | Fixed height error during 2D positioning                                                                      |
| 24   | R4        | -     | pDop         | -     | Position DOP maximum value                                                                                    |
| 28   | R4        | -     | tDop         | -     | Time DOP maximum value                                                                                        |
| 32   | R4        | -     | pAcc         | m^2   | The maximum value of position accuracy                                                                        |
| 36   | R4        | -     | tAcc         | m^2   | The maximum value of time accuracy                                                                            |
| 40   | R4        | -     | staticHoldTh | m/s   | Maintain the stationary threshold                                                                             |
Note 1: Parameter mask

| Value | Description                                                   |
| ----- | ------------------------------------------------------------- |
| B0    | Apply dynamic mode settings                                   |
| B1    | Apply positioning mode settings                               |
| B2    | Apply the maximum/minimum navigation satellite number setting |
| B3    | Apply the minimum signal-to-noise ratio setting               |
| B4    | Reserved                                                      |
| B5    | Apply initial positioning 3D settings                         |
| B6    | Apply the minimum elevation angle setting                     |
| B7    | Apply DR restriction settings                                 |
| B8    | Apply navigation system to enable                             |
| B9    | Apply GPS week rollover settings                              |
| B10   | Apply Altitude settings                                       |
| B11   | Apply location DOP restrictions                               |
| B12   | Application time DOP limit                                    |
| B13   | Apply static position hold settings                           |
Note 2: Dynamic mode

| Value | Description                  |
| ----- | ---------------------------- |
| 0     | Portable mode                |
| 1     | Still mode                   |
| 2     | Walking mode                 |
| 3     | Vehicle mode                 |
| 4     | Navigation mode              |
| 5     | Flight mode acceleration <1g |
| 6     | Flight mode acceleration <2g |
| 7     | Flight mode acceleration <3g |
Note 3: Positioning mode

| Value | Description                           |
| ----- | ------------------------------------- |
| 0     | Reserved                              |
| 1     | 2D Position                           |
| 2     | 3D positioning                        |
| 3     | 2D/3D positioning automatic switching |
Note 4: Nav System Toggles

| Value | Description  |
| ----- | ------------ |
| B0    | 1 = GPS      |
| B1    | 1 = BDS      |
| B2    | 1 = GLONASS  |
### 2.11.9 CFG-GROUP (0x06 0x08)
Query information

| Name              | CFG-GROUP                        |
| ----------------- | -------------------------------- |
| Description       | Query the group delay of GLONASS |
| Transmission Type | Query                            |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 0              | 0x06 0x08  | See Table Below | 4 bytes  |
Set information

| Name              | CFG-GROUP                            |
| ----------------- | ------------------------------------ |
| Description       | Configure the group delay of GLONASS |
| Transmission Type | Setup                                |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 56             | 0x06 0x08  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                                                                                                                                         |
| ---- | --------- | ----- | ---------- | ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0    | R4 \[14\] | -     | groupDelay | m     | GLONASS The group delay corresponding to each frequency is characterized by distance (the group delay time multiplied by the speed of light to obtain the distance) |

### 2.11.10 CFG-INS (0x06 0x10)
Query information

| Name              | CFG-INS                            |
| ----------------- | ---------------------------------- |
| Description       | Query the installation mode of INS |
| Transmission Type | Query                              |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 0              | 0x06 0x10  | See Table Below | 4 bytes  |
Set information

| Name              | CFG-INS                |
| ----------------- | ---------------------- |
| Description       | Configure the INS mode |
| Transmission Type | Setup                  |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 4              | 0x06 0x10  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| ---- | --------- | ----- | ---------- | ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0    | U2        | -     | attMode    | -     | The module is relative to the mode configuration of the vehicle's relative installation posture, and the possible value range is: 0, 1, 2 and 3.<br>0: The module X axis points to the front of the vehicle.<br>1: The X axis of the module points to the right side of the vehicle.<br>2: The X axis of the module points to the back of the vehicle.<br>3: The module X axis points to the left side of the vehicle.<br>9: Adaptive estimation module relative posture. The default is 9. |
| 2    | U2        | -     | ramStart   | -     | 1: The backup power is turned on immediately and the flight position calculation function is turned on.<br>0: The backup power is turned on and the flight position calculation function is turned off immediately.<br>Turned off by default                                                                                                                                                                                                                                                |
## 2.12 MSG (0x08)
The message of the receiver navigation message is 0x08.

### 2.12.1 MSG-BDSUTC (0x08 0x00)

| Name              | MSG-BDSUTC                                                       |
| ----------------- | ---------------------------------------------------------------- |
| Description       | BDS fixed point UTC data (synchronized with UTC time parameters) |
| Transmission Type | Periodic                                                         |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 20             | 0x08 0x00  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                                                                                            |
| ---- | --------- | ----- | ---------- | ----- | ---------------------------------------------------------------------------------------------------------------------- |
| 0    | U4        | -     | res1       | -     | Reserved                                                                                                               |
| 4    | I4        | 2^-30 | a0UTC      | s     | The clock difference between BDT and UTC                                                                               |
| 8    | I4        | 2^-30 | a1UTC      | s/s   | The clock speed of BDT relative to UTC                                                                                 |
| 12   | I1        | -     | dtls       | s     | Before the new leap seconds take effect, BDT changes the cumulative leap seconds relative to UTC. Positive Number      |
| 13   | I1        | -     | dtlsf      | s     | After the new leap seconds take effect, BDT will change the cumulative leap seconds compared with UTC. Positive Number |
| 14   | U1        | -     | res2       | -     | Reserved                                                                                                               |
| 15   | U1        | -     | res3       | -     | Reserved                                                                                                               |
| 16   | U1        | -     | wnlsf      | week  | The weekly count of the new leap seconds                                                                               |
| 17   | U1        | -     | dn         | day   | The new leap seconds take effect during the week                                                                       |
| 18   | U1        | -     | valid      | -     | Information validity (Note 1)                                                                                          |
| 19   | U1        | -     | res4       | -     | Reserved                                                                                                               |
Note 1: Information Validity

| Value | Description |
| ----- | ----------- |
| 0     | Invalid     |
| 1     | Unhealthy   |
| 2     | Expired     |
| 3     | Valid       |
### 2.12.2 MSG-BDSION (0x08 0x01)

| Name              | MSG-BDSION                                 |
| ----------------- | ------------------------------------------ |
| Description       | BDS8 parameter fixed point ionosphere data |
| Transmission Type | Periodic                                   |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 16             | 0x08 0x01  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units             | Description                              |
| ---- | --------- | ----- | ---------- | ----------------- | ---------------------------------------- |
| 0    | U4        | -     | res1       | -                 | Reserved                                 |
| 4    | I1        | 2^-30 | alpha0     | s                 | The clock difference between BDT and UTC |
| 5    | I1        | 2^-27 | alpha1     | $\frac{s}{\pi}$   | Ionosphere parameters                    |
| 6    | I1        | 2^-24 | alpha2     | $\frac{s}{\pi^2}$ | Ionosphere parameters                    |
| 7    | I1        | 2^-24 | alpha3     | $\frac{s}{\pi^3}$ | Ionosphere parameters                    |
| 8    | I1        | 2^11  | beta0      | s                 | Ionosphere parameters                    |
| 9    | I1        | 2^14  | beta1      | $\frac{s}{\pi}$   | Ionosphere parameters                    |
| 10   | I1        | 2^16  | beta2      | $\frac{s}{\pi^2}$ | Ionosphere parameters                    |
| 11   | I1        | 2^16  | beta3      | $\frac{s}{\pi^3}$ | Ionosphere parameters                    |
| 12   | U1        | -     | valid      | -                 | Information validity (Note 1)            |
| 13   | U1        | -     | res2       | -                 | Reserved                                 |
| 14   | U2        | -     | res3       | -                 | Reserved                                 |
Note 1: Information Validity

| Value | Description |
| ----- | ----------- |
| 0     | Invalid     |
| 1     | Unhealthy   |
| 2     | Expired     |
| 3     | Valid       |
### 2.12.3 MSG-BDSEPH (0x08 0x02)

| Name              | MSG-BDSION    |
| ----------------- | ------------- |
| Description       | BDS Ephemeris |
| Transmission Type | Periodic      |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 92             | 0x08 0x02  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name     | Units           | Description                                                                       |
| ---- | --------- | ----- | -------------- | --------------- | --------------------------------------------------------------------------------- |
| 0    | U4        | -     | res1           | -               | Reserved                                                                          |
| 4    | U4        | 2^-19 | sqra           | $m^{1/2}$       | The square root of the semi-long axis of the satellite orbit                      |
| 8    | U4        | 2^-33 | es             | -               | Eccentricity of satellite orbit                                                   |
| 12   | I4        | 2^-31 | $\omega$       | $\pi$           | Near point width angle                                                            |
| 16   | I4        | 2^-31 | $M_0$          | $\pi$           | The nearest angle of reference time                                               |
| 20   | I4        | 2^-31 | $i_0$          | $\pi$           | Orbital inclination of reference time                                             |
| 24   | I4        | 2^-31 | $\Omega_0$     | $\pi$           | The ascending intersection red meridian calculated by reference time              |
| 28   | I4        | 2^-43 | $\dot{\Omega}$ | $\frac{\pi}{s}$ | The rate of change of the red meridian at the ascending intersection              |
| 32   | I2        | 2^-43 | $\Delta n$     | $\frac{\pi}{s}$ | The difference between the average satellite motion rate and the calculated value |
| 34   | I2        | 2^-43 | IDOT           | $\frac{\pi}{s}$ | Change rate of orbital inclination                                                |
| 36   | I4        | 2^-31 | cuc            | rad             | Cosine tuning and correction term amplitude of latitude amplitude                 |
| 40   | I4        | 2^-31 | cus            | rad             | The sine tuning and correction term amplitude of the latitude amplitude           |
| 44   | I4        | 2^-6  | crc            | m               | Cosine tuning and correction term amplitude of orbital radius                     |
| 48   | I4        | 2^-6  | crs            | rad             | Sine modulation and correction term amplitude of the orbital radius               |
| 52   | I4        | 2^-31 | cic            | rad             | Cosine tuning and correction amplitude of orbital inclination                     |
| 56   | I4        | 2^-31 | cis            | s               | Sine tuning and correction term amplitude of orbital inclination                  |
| 60   | U4        | 2^3   | toe            | -               | Ephemeris reference time                                                          |
| 64   | U2        | -     | wne            | -               | Week Number Reference Time                                                        |
| 66   | U2        | -     | res2           | -               | Reserved                                                                          |
| 68   | U4        | 2^3   | toc            | s               | Reference time of the clock difference parameters of this period                  |
| 72   | I4        | 2^-33 | af0            | s               | Satellite ranging code phase time offset coefficient                              |
| 76   | I4        | 2^-50 | af1            | s/s             | Satellite ranging code phase time offset coefficient                              |
| 80   | I2        | 2^-66 | af2            | $s/s^2$         | Satellite ranging code phase time offset coefficient                              |
| 82   | I2        | 0.1   | tgd            | ns              | Time delay on the receiver                                                        |
| 84   | U1        | -     | iodc           | -               | Clock data age                                                                    |
| 85   | U1        | -     | iode           | -               | Ephemeris data age period                                                         |
| 86   | U1        | -     | ura            | -               | User distance accuracy                                                            |
| 87   | U1        | -     | health         | -               | Satellite autonomous health                                                       |
| 88   | U1        | -     | svid           | -               | Satellite number                                                                  |
| 89   | U1        | -     | valid          | -               | Information Validity (Note 1)                                                     |
| 90   | U2        | -     | res3           | -               | Reserved                                                                          |

Note 1: Information Validity

| Value | Description |
| ----- | ----------- |
| 0     | Invalid     |
| 1     | Unhealthy   |
| 2     | Expired     |
| 3     | Valid       |
### 2.12.4 MSG-GPSUTC (0x08 0x05)

| Name              | MSG-GPSUTC                                                    |
| ----------------- | ------------------------------------------------------------- |
| Description       | GPS fixed UTC data (synchronization parameters with UTC time) |
| Transmission Type | Periodic                                                      |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 20             | 0x08 0x05  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description                                                                                                            |
| ---- | --------- | ----- | ---------- | ----- | ---------------------------------------------------------------------------------------------------------------------- |
| 0    | U4        | -     | res1       | -     | Reserved                                                                                                               |
| 4    | I4        | 2^-30 | a0UTC      | s     | The clock difference between BDT and UTC                                                                               |
| 8    | I4        | 2^-30 | a1UTC      | s/s   | The clock speed of BDT relative to UTC                                                                                 |
| 12   | I1        | -     | dtls       | s     | Before the new leap seconds take effect, BDT changes the cumulative leap seconds relative to UTC. Positive Number      |
| 13   | I1        | -     | dtlsf      | s     | After the new leap seconds take effect, BDT will change the cumulative leap seconds compared with UTC. Positive Number |
| 14   | U1        | -     | tot        | s     | UTC The reference time of the data                                                                                     |
| 15   | U1        | -     | wnt        | week  | UTC Number of weeks                                                                                                    |
| 16   | U1        | -     | wnlsf      | week  | The weekly count of the new leap seconds                                                                               |
| 17   | U1        | -     | dn         | day   | The new leap seconds take effect during the week                                                                       |
| 18   | U1        | -     | valid      | -     | Information validity (Note 1)                                                                                          |
| 19   | U1        | -     | res2       |       |                                                                                                                        |
Note 1: Information Validity

| Value | Description |
| ----- | ----------- |
| 0     | Invalid     |
| 1     | Unhealthy   |
| 2     | Expired     |
| 3     | Valid       |
### 2.12.5 MSG-GPSION (0x08 0x06)

| Name              | MSG-GPSION                                |
| ----------------- | ----------------------------------------- |
| Description       | GPS parameter fixed point ionosphere data |
| Transmission Type | Periodic                                  |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 16             | 0x08 0x06  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units             | Description                              |
| ---- | --------- | ----- | ---------- | ----------------- | ---------------------------------------- |
| 0    | U4        | -     | res1       | -                 | Reserved                                 |
| 4    | I1        | 2^-30 | alpha0     | s                 | The clock difference between BDT and UTC |
| 5    | I1        | 2^-27 | alpha1     | $\frac{s}{\pi}$   | Ionosphere parameters                    |
| 6    | I1        | 2^-24 | alpha2     | $\frac{s}{\pi^2}$ | Ionosphere parameters                    |
| 7    | I1        | 2^-24 | alpha3     | $\frac{s}{\pi^3}$ | Ionosphere parameters                    |
| 8    | I1        | 2^11  | beta0      | s                 | Ionosphere parameters                    |
| 9    | I1        | 2^14  | beta1      | $\frac{s}{\pi}$   | Ionosphere parameters                    |
| 10   | I1        | 2^16  | beta2      | $\frac{s}{\pi^2}$ | Ionosphere parameters                    |
| 11   | I1        | 2^16  | beta3      | $\frac{s}{\pi^3}$ | Ionosphere parameters                    |
| 12   | U1        | -     | valid      | -                 | Information validity (Note 1)            |
| 13   | U1        | -     | res2       | -                 | Reserved                                 |
| 14   | U2        | -     | res3       | -                 | Reserved                                 |
Note 1: Information Validity

| Value | Description |
| ----- | ----------- |
| 0     | Invalid     |
| 1     | Unhealthy   |
| 2     | Expired     |
| 3     | Valid       |
### 2.12.3 MSG-GPSEPH (0x08 0x07)

| Name              | MSG-GPSEPH    |
| ----------------- | ------------- |
| Description       | GPS Ephemeris |
| Transmission Type | Periodic      |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 72             | 0x08 0x07  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name     | Units           | Description                                                                       |
| ---- | --------- | ----- | -------------- | --------------- | --------------------------------------------------------------------------------- |
| 0    | U4        | -     | res1           | -               | Reserved                                                                          |
| 4    | U4        | 2^-19 | sqra           | $m^{1/2}$       | The square root of the semi-long axis of the satellite orbit                      |
| 8    | U4        | 2^-33 | es             | -               | Eccentricity of satellite orbit                                                   |
| 12   | I4        | 2^-31 | $\omega$       | $\pi$           | Near point width angle                                                            |
| 16   | I4        | 2^-31 | $M_0$          | $\pi$           | The nearest angle of reference time                                               |
| 20   | I4        | 2^-31 | $i_0$          | $\pi$           | Orbital inclination of reference time                                             |
| 24   | I4        | 2^-31 | $\Omega_0$     | $\pi$           | The ascending intersection red meridian calculated by reference time              |
| 28   | I4        | 2^-43 | $\dot{\Omega}$ | $\frac{\pi}{s}$ | The rate of change of the red meridian at the ascending intersection              |
| 32   | I2        | 2^-43 | $\Delta n$     | $\frac{\pi}{s}$ | The difference between the average satellite motion rate and the calculated value |
| 34   | I2        | 2^-43 | IDOT           | $\frac{\pi}{s}$ | Change rate of orbital inclination                                                |
| 36   | I2        | 2^-29 | cuc            | rad             | Cosine tuning and correction term amplitude of latitude amplitude                 |
| 38   | I2        | 2^-29 | cus            | rad             | The sine tuning and correction term amplitude of the latitude amplitude           |
| 40   | I2        | 2^-5  | crc            | m               | Cosine tuning and correction term amplitude of orbital radius                     |
| 42   | I2        | 2^-5  | crs            | m               | Sine modulation and correction term amplitude of the orbital radius               |
| 44   | I2        | 2^-29 | cic            | rad             | Cosine tuning and correction amplitude of orbital inclination                     |
| 46   | I2        | 2^-29 | cis            | rad             | Sine tuning and correction term amplitude of orbital inclination                  |
| 48   | U2        | 2^4   | toe            | s               | Ephemeris reference time                                                          |
| 50   | U2        | -     | wne            | -               | Week Number Reference Time                                                        |
| 52   | U4        | 2^4   | toc            | s               | Reference time of the clock difference parameters of this period                  |
| 56   | I4        | 2^-31 | af0            | s               | Satellite ranging code phase time offset coefficient                              |
| 60   | I2        | 2^-43 | af1            | s/s             | Satellite ranging code phase time offset coefficient                              |
| 62   | I1        | 2^-55 | af2            | $s/s^2$         | Satellite ranging code phase time offset coefficient                              |
| 63   | I1        | 2^-31 | tgd            | s               | Time delay on the receiver                                                        |
| 64   | U2        | -     | iodc           | -               | Clock data age                                                                    |
| 66   | U1        | -     | ura            | -               | User distance accuracy                                                            |
| 67   | U1        | -     | health         | -               | Satellite autonomous health                                                       |
| 68   | U1        | -     | svid           | -               | Satellite number                                                                  |
| 69   | U1        | -     | valid          | -               | Information Validity (Note 1)                                                     |
| 70   | U2        | -     | res2           | -               | Reserved                                                                          |

Note 1: Information Validity

| Value | Description |
| ----- | ----------- |
| 0     | Invalid     |
| 1     | Unhealthy   |
| 2     | Expired     |
| 3     | Valid       |
### 2.12.3 MSG-GLNEPH (0x08 0x08)

| Name              | MSG-GLNEPH        |
| ----------------- | ----------------- |
| Description       | GLONASS Ephemeris |
| Transmission Type | Periodic          |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 68             | 0x08 0x08  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units    | Description                                                                             |
| ---- | --------- | ----- | ---------- | -------- | --------------------------------------------------------------------------------------- |
| 0    | U4        | -     | res1       | -        | Reserved                                                                                |
| 4    | I4        | 2^-30 | taon       | s        | The modified value of the *nth* satellite relative to GLONASS time                      |
| 8    | I4        | 2^-11 | x          | km       | Satellite position coordinates in the PZ-90 coordinate system                           |
| 12   | I4        | 2^-11 | y          | km       | Satellite position coordinates in the PZ-90 coordinate system                           |
| 16   | I4        | 2^-11 | z          | km       | Satellite position coordinates in the PZ-90 coordinate system                           |
| 20   | I4        | 2^-20 | dx         | km/s     | Satellite speed in PZ-90 coordinate system                                              |
| 24   | I4        | 2^-20 | dy         | km/s     | Satellite speed in PZ-90 coordinate system                                              |
| 28   | I4        | 2^-20 | dz         | km/s     | Satellite speed in PZ-90 coordinate system                                              |
| 32   | I4        | 2^-31 | taoc       | s        | GLONASS time relative UTC time scale correction                                         |
| 36   | I4        | 2^-30 | taoGPS     | day      | Correction from GLONASS time to GPS time                                                |
| 40   | I2        | 2^-40 | gamman     | -        | Relative deviation of satellite prediction carrier frequency                            |
| 42   | U2        | -     | tk         | -        | Within the day of the current frame, a total of 12bit                                   |
| 44   | U2        | -     | nt         | day      | The current date of counting from January of the previous leap year                     |
| 46   | I1        | 2^-30 | ddx        | $km/s^2$ | Satellite acceleration in PZ-90 coordinate system                                       |
| 47   | I1        | 2^-30 | ddy        | $km/s^2$ | Satellite acceleration in PZ-90 coordinate system                                       |
| 48   | I1        | 2^-30 | ddz        | $km/s^2$ | Satellite acceleration in PZ-90 coordinate system                                       |
| 49   | I1        | 2^-30 | dtaon      | s        | The propagation time difference between the *nth* satellite L2 signal and the L1 signal |
| 50   | U1        | -     | bn         | -        | Health Information                                                                      |
| 51   | U1        | 900   | tb         | s        | The intraday time of the current time (subject to UTC+3)                                |
| 52   | U1        | -     | M          | -        | GLONASS satellite category                                                              |
| 53   | U1        | -     | P          | -        | Technical Control parameters                                                            |
| 54   | U1        | -     | ft         | -        | Prediction accuracy of satellite pseudo-distance                                        |
| 55   | U1        | -     | en         | day      | The age of ephemeris                                                                    |
| 56   | U1        | -     | p1         | -        | Ephemeris information update time mark bit                                              |
| 57   | U1        | -     | p2         | -        | User distance accuracy                                                                  |
| 58   | U1        | -     | p3         | -        | The ephemeris transmitted by the current frame includes the number of satellites.       |
| 59   | u1        | -     | p4         | -        | Ephemeris data update indicator: 1 is updated                                           |
| 60   | U1        | -     | ln         | -        | Satellite Health Indicator (GLONASS-M Satellite)                                        |
| 61   | U1        | -     | n4         | -        | Time counting (starting from 1996, with a four-year cycle)                              |
| 62   | U1        | -     | svid       | -        | Satellite number                                                                        |
| 63   | U1        | -     | nl         | -        | Frequency number                                                                        |
| 64   | U1        | -     | valid      | -        | Information Validity (Note 1)                                                           |
| 65   | U1        | -     | res2       | -        | Reserved                                                                                |
| 66   | U2        | -     | res3       | -        | Reserved                                                                                |

Note 1: Information Validity

| Value | Description |
| ----- | ----------- |
| 0     | Invalid     |
| 1     | Unhealthy   |
| 2     | Expired     |
| 3     | Valid       |
## 2.13 MON (0x0A)
Monitor information, such as configuration status, task status, etc.
### 2.13.1 MON-VER (0x0A 0x04)

| Name              | MON-VER             |
| ----------------- | ------------------- |
| Description       | Version information |
| Transmission Type | Query Response      |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 64             | 0x0A 0x04  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units | Description             |
| ---- | --------- | ----- | ---------- | ----- | ----------------------- |
| 0    | CH\[32\]  | -     | swVersion  | -     | Software version string |
| 32   | CH\[32\]  | -     | hwVersion  | -     | Hardware version string |
### 2.13.2 MON-HW (0x0A 0x09)

| Name              | MON-HW                                                                                                                 |
| ----------------- | ---------------------------------------------------------------------------------------------------------------------- |
| Description       | Hardware status                                                                                                        |
| Transmission Type | Periodic                                                                                                               |
| Note              | Various configuration states of hardware, including antenna status, IO port status, noise level, AGC information, etc. |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 56             | 0x0A 0x09  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name  | Units | Description                                                                   |
| ---- | --------- | ----- | ----------- | ----- | ----------------------------------------------------------------------------- |
| 0    | U4        | -     | noisePerMs0 | -     | DIF0 Noise power of intermediate frequency data                               |
| 4    | U4        | -     | noisePerMs1 | -     | DIF1 Noise power of intermediate frequency data                               |
| 8    | U4        | -     | noisePerMs2 | -     | DIF2 Noise power of intermediate frequency data                               |
| 12   | U2        | -     | agcData0    | -     | DIF0 The number of 1 of the amplitude bits of the intermediate frequency data |
| 14   | U2        | -     | agcData1    | -     | DIF1 The number of 1 of the amplitude bits of the intermediate frequency data |
| 16   | U2        | -     | agcData2    | -     | DIF2 The number of 1 of the amplitude bits of the intermediate frequency data |
| 18   | U2        | -     | res         | -     | Reserved                                                                      |
| 20   | U1        | -     | antStatus   | -     | Antenna status (Note 1)                                                       |
| 21   | U1        | -     | res         | -     | Reserved                                                                      |
| 22   | U1        | -     | res         | -     | Reserved                                                                      |
| 23   | U1        | -     | res         | -     | Reserved                                                                      |
| 24   | U4\[8\]   | 2^24  | jamming     | -     | The central frequency of the interference signal (interalized)                |
Note 1: Antenna status

| Value | Description    |
| ----- | -------------- |
| 0     | Initialization |
| 1     | Unknown State  |
| 2     | Normal         |
| 3     | Antenna Short  |
| 4     | Antenna Open   |
## 2.14 AID (0x0B)
Auxiliary information, such as the initial location of the receiver, time, etc.### 2.13.2 
### AID-INI (0x0B 0x01)

| Name              | AID-INI                                                               |
| ----------------- | --------------------------------------------------------------------- |
| Description       | Auxiliary position, time, frequency, clock frequency bias information |
| Transmission Type | Query/Input                                                           |
| Note              | Configure navigation-related parameters                               |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 56             | 0x0B 0x01  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units      | Description                                                                                                                                                                           |
| ---- | --------- | ----- | ---------- | ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0    | R8        | -     | ecefXOrLat | m or 1 deg | X coordinates or latitude:<br>In the ECEF coordinate system, if it is the ECEF coordinate system, the unit is m.<br>If it is latitude, the unit is degree.                            |
| 8    | R8        | -     | ecefYOrLat | m or 1 deg | Y coordinates or longitude:<br>In the ECEF coordinate system, if it is the ECEF coordinate system, the unit is m.<br>If it is longitude, the unit is degree.                          |
| 16   | R8        | -     | ecefZOrAlt | m          | Z coordinates or altitude:<br>In the ECEF coordinate system, if it is the ECEF coordinate system, the unit is m.<br>If it is altitude, the unit is meters.                            |
| 24   | R8        | -     | tow        | s          | GPS Time of Week                                                                                                                                                                      |
| 32   | R4        | 300   | freqBias   | ppm        | Clock frequency drift. For example: FreqBias=300, indicating that the crystal frequency is biased by 1ppm; FreqBias=-150, indicating that the crystal frequency is biased by -0.5ppm; |
| 36   | R4        | -     | pAcc       | m^2        | The variance of the estimated error of 3D position                                                                                                                                    |
| 40   | R4        | C^2   | tAcc       | s^2        | The variance of the estimated error of time. For example: tAcc = 9, the time difference shown is sqrt(tAcc)/C = 3/3e8 = 10ns                                                          |
| 44   | R4        | 300^2 | fAcc       | ppm^2      | The variance of the clock frequency drift error. For example: fAcc = 900, the time difference is sqrt(fAcc)/300 = 30/300 = 0.1ppm                                                     |
| 48   | U4        | -     | res        | -          | Reserved                                                                                                                                                                              |
| 52   | U2        | -     | wn         | -          | GPS Week Number                                                                                                                                                                       |
| 54   | U1        | -     | timeSource | -          | Source of time                                                                                                                                                                        |
| 55   | U1        | -     | flags      | -          | Flags (Note 1)                                                                                                                                                                        |
Note 1: Flags

| Value | Description                                 |
| ----- | ------------------------------------------- |
| B0    | 1 = Valid position                          |
| B1    | 1 = Time is valid                           |
| B2    | 1 = Clock frequency drift data is effective |
| B3    | Reserved                                    |
| B4    | 1 = Clock frequency data is valid           |
| B5    | 1 = The position is in LLA format           |
| B6    | 1 = Invalid height                          |
| B7    | Reserved                                    |
### AID-HUI (0x0B 0x03)

| Name              | AID-HUI                                                             |
| ----------------- | ------------------------------------------------------------------- |
| Description       | Auxiliary health information, UTC parameters, ionosphere parameters |
| Transmission Type | Input                                                               |
| Note              | Configure navigation-related parameters                             |

Data Structure

| Header    | Length (bytes) | Identifier | Payload         | Checksum |
| --------- | -------------- | ---------- | --------------- | -------- |
| 0xBA 0xCE | 60             | 0x0B 0x03  | See Table Below | 4 bytes  |

Payload Information

| Byte | Data Type | Scale | Field Name | Units     | Description                                                                   |
| ---- | --------- | ----- | ---------- | --------- | ----------------------------------------------------------------------------- |
| 0    | U4        | -     | -          | -         | The datasheet did not list anything for Byte 0, which is pretty strange.      |
| 4    | U4        | -     | HeaGPS     | -         | Health information of GPS satellite (Note 1)                                  |
| 8    | U4        | -     | HeaBds     | -         | Health information of BDS satellite (Note 1)                                  |
| 12   | U4        | -     | HeaGln     | -         | Health information of GLONASS satellite (Note 1)                              |
| 16   | I4        | 2^-30 | utcGpsA0   | s         | UTC parameter A0, GPS clock difference relative to UTC                        |
| 20   | I4        | 2^-50 | utcGpsA1   | s/s       | UTC parameter A1, GPS clock difference relative to UTC                        |
| 24   | I1        | -     | utcGpsLS   | s         | The new leap second jump before the GPS relative to UTC                       |
| 25   | I1        | -     | utcGpsLSF  | s         | After the new jump second, the leap second jump of GPS relative to UTC        |
| 26   | U1        | -     | utcGpsTow  | s         | Reference week time of UTC parameters of GPS                                  |
| 27   | U1        | -     | utcGpsWNT  | week      | Reference week number of UTC parameters of GPS                                |
| 28   | U1        | -     | utcGpsWNF  | week      | GPS Week Number when the new jump seconds take effect                         |
| 29   | U1        | -     | utcGpsDN   | day       | The number of days within the week when the new GPS leap seconds takes effect |
| 30   | I2        | -     | Res        | -         | Reserved                                                                      |
| 32   | I4        | 2^-30 | utcBdsA0   | s         | UTC parameter A0, BDS clock difference relative to UTC                        |
| 36   | I4        | 2^-50 | utcBdsA1   | s/s       | UTC parameter A1, BDS clock difference relative to UTC                        |
| 40   | I1        | -     | utcBdsLS   | s         | The new leap second jump before the GPS relative to UTC                       |
| 41   | I1        | -     | utcBdsLSF  | s         | After the new jump second, the leap second jump of GPS relative to UTC        |
| 42   | U1        | -     | utcBdsTow  | s         | Reference week time of UTC parameters of BDS                                  |
| 43   | U1        | -     | utcBdsWNT  | week      | Reference week number of UTC parameters of BDS                                |
| 44   | U1        | -     | utcBdsWNF  | week      | BDS Week Number when the new jump seconds take effect                         |
| 45   | U1        | -     | utcBdsDN   | day       | The number of days within the week when the new BDS leap seconds takes effect |
| 46   | I2        | -     | Res        | -         | Reserved                                                                      |
| 48   | I1        | 2^-30 | klobA0     | $s/\pi^2$ | Klobuchar model parameters alpha0                                             |
| 49   | I1        | 2^-27 | klobA1     | $s/\pi^1$ | Klobuchar model parameters alpha1                                             |
| 50   | I1        | 2^-24 | klobA2     | $s/\pi^2$ | Klobuchar model parameters alpha2                                             |
| 51   | I1        | 2^-24 | klobA3     | $s/\pi^3$ | Klobuchar model parameters alpha3                                             |
| 52   | I1        | 2^11  | klobB0     | $s/\pi$   | Klobuchar model parameters beta0                                              |
| 53   | I1        | 2^14  | klobB1     | $s/\pi^1$ | Klobuchar model parameters beta1                                              |
| 54   | I1        | 2^16  | klobB2     | $s/\pi^2$ | Klobuchar model parameters beta2                                              |
| 55   | I1        | 2^16  | klobB3     | $s/\pi^3$ | Klobuchar model parameters beta3                                              |
| 56   | U4        | -     | flags      | -         | Status Flags (Note 2)                                                         |
Note 1: B0 represents the No. 1 satellite, and the corresponding bit is equal to 0, indicating the health of the satellite.

Note 2: Validity

| Value | Description                                    |
| ----- | ---------------------------------------------- |
| B0    | 1 = Health parameters valid                    |
| B1    | 1 = UTC Time is valid                          |
| B2    | 1 = The parameters of the ionosphere are valid |

