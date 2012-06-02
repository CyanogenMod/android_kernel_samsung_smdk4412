/******************************************/
/* Synaptics Design Studio 4          3.0.1.7 */
/*     2012-01-09 오후 7:14:06                  */
/******************************************/

/**** Device Information ****/
#define FW_REVISION		"DS4 R3.0"	/*  F01 Firmware ProductID  */
#define TOUCH_CONTROLLER	"S7301-I2C"	/*  F01 PackageID  */
#define CONFIG_ID		"E002"		/*  F34 Configuration ID  */
#define FW_PACKRAT		"1042884"	/*  Firmware Packrat  */

/**** Register names ****/
#define F34_FLASH_CTRL00_00	0x004B	/*  Customer Defined Config ID 0 */
#define F34_FLASH_CTRL00_01	0x004C	/*  Customer Defined Config ID 1 */
#define F34_FLASH_CTRL00_02	0x004D	/*  Customer Defined Config ID 2 */
#define F34_FLASH_CTRL00_03	0x004E	/*  Customer Defined Config ID 3 */
#define F01_RMI_CTRL00		0x004F	/*  Device Control		*/
#define F01_RMI_CTRL01_00	0x0050	/*  Interrupt Enable 0		*/
#define F01_RMI_CTRL02		0x0051	/*  Doze Period	*/
#define F01_RMI_CTRL03		0x0052	/*  Doze Wakeup Threshold	*/
#define F01_RMI_CTRL05		0x0053	/*  Extended Wait Before Doze	*/
#define F01_RMI_CTRL06		0x0054	/*  I2C Control	*/
#define F01_RMI_CTRL08		0x0055	/*  Attention Control		*/
#define F11_2D_CTRL00		0x0056	/*  2D Report Mode		*/
#define F11_2D_CTRL01		0x0057	/*  2D Palm Detect		*/
#define F11_2D_CTRL02		0x0058	/*  2D Delta-X Thresh		*/
#define F11_2D_CTRL03		0x0059	/*  2D Delta-Y Thresh		*/
#define F11_2D_CTRL04		0x005A	/*  2D Velocity	*/
#define F11_2D_CTRL05		0x005B	/*  2D Acceleration		*/
#define F11_2D_CTRL06		0x005C	/*  2D Max X Position (7:0)	*/
#define F11_2D_CTRL07		0x005D	/*  2D Max X Position (11:8)	*/
#define F11_2D_CTRL08		0x005E	/*  2D Max Y Position (7:0)	*/
#define F11_2D_CTRL09		0x005F	/*  2D Max Y Position (11:8)	*/
#define F11_2D_CTRL29		0x0060	/*  Z Touch Threshold		*/
#define F11_2D_CTRL30		0x0061	/*  Z Touch Hysteresis	*/
#define F11_2D_CTRL31		0x0062	/*  Small Z Threshold		*/
#define F11_2D_CTRL32_00	0x0063	/*  Small Z Scale Factor	*/
#define F11_2D_CTRL32_01	0x0064	/*  Small Z Scale Factor	*/
#define F11_2D_CTRL33_00	0x0065	/*  Large Z Scale Factor	*/
#define F11_2D_CTRL33_01	0x0066	/*  Large Z Scale Factor	*/
#define F11_2D_CTRL34		0x0067	/*  Position Calculation &
						Post Correction		*/
#define F11_2D_CTRL36		0x0068	/*  Wx Scale Factor		*/
#define F11_2D_CTRL37		0x0069	/*  Wx Offset			*/
#define F11_2D_CTRL38		0x006A	/*  Wy Scale Factor		*/
#define F11_2D_CTRL39		0x006B	/*  Wy Offset			*/
#define F11_2D_CTRL40_00	0x006C	/*  X Pitch			*/
#define F11_2D_CTRL40_01	0x006D	/*  X Pitch			*/
#define F11_2D_CTRL41_00	0x006E	/*  Y Pitch			*/
#define F11_2D_CTRL41_01	0x006F	/*  Y Pitch			*/
#define F11_2D_CTRL42_00	0x0070	/*  Default Finger Width Tx	*/
#define F11_2D_CTRL42_01	0x0071	/*  Default Finger Width Tx	*/
#define F11_2D_CTRL43_00	0x0072	/*  Default Finger Width Ty	*/
#define F11_2D_CTRL43_01	0x0073	/*  Default Finger Width Ty	*/
#define F11_2D_CTRL44		0x0074	/*  Report Finger Width		*/
#define F11_2D_CTRL45		0x0075	/*  Segmentation Aggressiveness */
#define F11_2D_CTRL46		0x0076	/*  Rx Clip Low			*/
#define F11_2D_CTRL47		0x0077	/*  Rx Clip High		*/
#define F11_2D_CTRL48		0x0078	/*  Tx Clip Low			*/
#define F11_2D_CTRL49		0x0079	/*  Tx Clip High		*/
#define F11_2D_CTRL50		0x007A	/*  Minimum Finger Separation	*/
#define F11_2D_CTRL51		0x007B	/*  Maximum Finger Movement	*/
#define F11_2D_CTRL58		0x007C	/*  Large Object
						Suppression Parameters	*/
#define F11_2D_CTRL73		0x007D	/*  Jitter Control 1		*/
#define F11_2D_CTRL74		0x007E	/*  Jitter Control 2		*/
#define F11_2D_CTRL75_00	0x007F	/*  Jitter Control 3		*/
#define F11_2D_CTRL75_01	0x0080	/*  Jitter Control 3		*/
#define F11_2D_CTRL75_02	0x0081	/*  Jitter Control 3		*/
#define F11_2D_CTRL76_00	0x0082	/*  Jitter Control 4		*/
#define F11_2D_CTRL76_01	0x0083	/*  Jitter Control 4		*/
#define F11_2D_CTRL76_02	0x0084	/*  Jitter Control 4		*/
#define F54_ANALOG_CTRL00	0x010B	/*  General Control		*/
#define F54_ANALOG_CTRL01	0x010C	/*  General Control 1		*/
#define F54_ANALOG_CTRL02_00	0x010D	/*  Saturation Capacitance Low  */
#define F54_ANALOG_CTRL02_01	0x010E	/*  Saturation Capacitance High */
#define F54_ANALOG_CTRL03	0x010F	/*  Pixel Touch Threshold	*/
#define F54_ANALOG_CTRL04	0x0110	/*  Misc Analog Control		*/
#define F54_ANALOG_CTRL05	0x0111	/*  RefCap RefLo Settings	*/
#define F54_ANALOG_CTRL06	0x0112	/*  RefCap RefHi Settings	*/
#define F54_ANALOG_CTRL07	0x0113	/*  CBC Cap Settings		*/
#define F54_ANALOG_CTRL08_00	0x0114	/*  Integration Duration Low	*/
#define F54_ANALOG_CTRL08_01	0x0115	/*  Integration Duration High	*/
#define F54_ANALOG_CTRL09	0x0116	/*  Reset Duration		*/
#define F54_ANALOG_CTRL10	0x0117	/*  Noise Measurement Control	*/
#define F54_ANALOG_CTRL11_00	0x0118	/*  Doze Wakeup Threshold Low	*/
#define F54_ANALOG_CTRL11_01	0x0119	/*  Doze Wakeup Threshold High  */
#define F54_ANALOG_CTRL12	0x011A	/*  Slow Relaxation Rate	*/
#define F54_ANALOG_CTRL13	0x011B	/*  Fast Relaxation Rate	*/
#define F54_ANALOG_CTRL14	0x011C	/*  Sensor Physical Properties  */
#define F54_ANALOG_CTRL15_00	0x011D	/*  Sensor Rx Mapping 0         */
#define F54_ANALOG_CTRL15_01	0x011E	/*  Sensor Rx Mapping 1         */
#define F54_ANALOG_CTRL15_02	0x011F	/*  Sensor Rx Mapping 2         */
#define F54_ANALOG_CTRL15_03	0x0120	/*  Sensor Rx Mapping 3         */
#define F54_ANALOG_CTRL15_04	0x0121	/*  Sensor Rx Mapping 4         */
#define F54_ANALOG_CTRL15_05	0x0122	/*  Sensor Rx Mapping 5         */
#define F54_ANALOG_CTRL15_06	0x0123	/*  Sensor Rx Mapping 6         */
#define F54_ANALOG_CTRL15_07	0x0124	/*  Sensor Rx Mapping 7         */
#define F54_ANALOG_CTRL15_08	0x0125	/*  Sensor Rx Mapping 8         */
#define F54_ANALOG_CTRL15_09	0x0126	/*  Sensor Rx Mapping 9         */
#define F54_ANALOG_CTRL15_10	0x0127	/*  Sensor Rx Mapping 10	*/
#define F54_ANALOG_CTRL15_11	0x0128	/*  Sensor Rx Mapping 11	*/
#define F54_ANALOG_CTRL15_12	0x0129	/*  Sensor Rx Mapping 12	*/
#define F54_ANALOG_CTRL15_13	0x012A	/*  Sensor Rx Mapping 13	*/
#define F54_ANALOG_CTRL15_14	0x012B	/*  Sensor Rx Mapping 14	*/
#define F54_ANALOG_CTRL15_15	0x012C	/*  Sensor Rx Mapping 15	*/
#define F54_ANALOG_CTRL15_16	0x012D	/*  Sensor Rx Mapping 16	*/
#define F54_ANALOG_CTRL15_17	0x012E	/*  Sensor Rx Mapping 17	*/
#define F54_ANALOG_CTRL15_18	0x012F	/*  Sensor Rx Mapping 18	*/
#define F54_ANALOG_CTRL15_19	0x0130	/*  Sensor Rx Mapping 19	*/
#define F54_ANALOG_CTRL15_20	0x0131	/*  Sensor Rx Mapping 20	*/
#define F54_ANALOG_CTRL15_21	0x0132	/*  Sensor Rx Mapping 21	*/
#define F54_ANALOG_CTRL15_22	0x0133	/*  Sensor Rx Mapping 22	*/
#define F54_ANALOG_CTRL15_23	0x0134	/*  Sensor Rx Mapping 23	*/
#define F54_ANALOG_CTRL15_24	0x0135	/*  Sensor Rx Mapping 24	*/
#define F54_ANALOG_CTRL15_25	0x0136	/*  Sensor Rx Mapping 25	*/
#define F54_ANALOG_CTRL15_26	0x0137	/*  Sensor Rx Mapping 26	*/
#define F54_ANALOG_CTRL15_27	0x0138	/*  Sensor Rx Mapping 27	*/
#define F54_ANALOG_CTRL15_28	0x0139	/*  Sensor Rx Mapping 28	*/
#define F54_ANALOG_CTRL15_29	0x013A	/*  Sensor Rx Mapping 29	*/
#define F54_ANALOG_CTRL15_30	0x013B	/*  Sensor Rx Mapping 30	*/
#define F54_ANALOG_CTRL15_31	0x013C	/*  Sensor Rx Mapping 31	*/
#define F54_ANALOG_CTRL15_32	0x013D	/*  Sensor Rx Mapping 32	*/
#define F54_ANALOG_CTRL15_33	0x013E	/*  Sensor Rx Mapping 33	*/
#define F54_ANALOG_CTRL15_34	0x013F	/*  Sensor Rx Mapping 34	*/
#define F54_ANALOG_CTRL15_35	0x0140	/*  Sensor Rx Mapping 35	*/
#define F54_ANALOG_CTRL15_36	0x0141	/*  Sensor Rx Mapping 36	*/
#define F54_ANALOG_CTRL15_37	0x0142	/*  Sensor Rx Mapping 37	*/
#define F54_ANALOG_CTRL15_38	0x0143	/*  Sensor Rx Mapping 38	*/
#define F54_ANALOG_CTRL15_39	0x0144	/*  Sensor Rx Mapping 39	*/
#define F54_ANALOG_CTRL15_40	0x0145	/*  Sensor Rx Mapping 40	*/
#define F54_ANALOG_CTRL15_41	0x0146	/*  Sensor Rx Mapping 41	*/
#define F54_ANALOG_CTRL15_42	0x0147	/*  Sensor Rx Mapping 42	*/
#define F54_ANALOG_CTRL15_43	0x0148	/*  Sensor Rx Mapping 43	*/
#define F54_ANALOG_CTRL15_44	0x0149	/*  Sensor Rx Mapping 44	*/
#define F54_ANALOG_CTRL15_45	0x014A	/*  Sensor Rx Mapping 45	*/
#define F54_ANALOG_CTRL16_00	0x014B	/*  Sensor Tx Mapping 0         */
#define F54_ANALOG_CTRL16_01	0x014C	/*  Sensor Tx Mapping 1         */
#define F54_ANALOG_CTRL16_02	0x014D	/*  Sensor Tx Mapping 2         */
#define F54_ANALOG_CTRL16_03	0x014E	/*  Sensor Tx Mapping 3         */
#define F54_ANALOG_CTRL16_04	0x014F	/*  Sensor Tx Mapping 4         */
#define F54_ANALOG_CTRL16_05	0x0150	/*  Sensor Tx Mapping 5         */
#define F54_ANALOG_CTRL16_06	0x0151	/*  Sensor Tx Mapping 6         */
#define F54_ANALOG_CTRL16_07	0x0152	/*  Sensor Tx Mapping 7         */
#define F54_ANALOG_CTRL16_08	0x0153	/*  Sensor Tx Mapping 8         */
#define F54_ANALOG_CTRL16_09	0x0154	/*  Sensor Tx Mapping 9         */
#define F54_ANALOG_CTRL16_10	0x0155	/*  Sensor Tx Mapping 10	*/
#define F54_ANALOG_CTRL16_11	0x0156	/*  Sensor Tx Mapping 11	*/
#define F54_ANALOG_CTRL16_12	0x0157	/*  Sensor Tx Mapping 12	*/
#define F54_ANALOG_CTRL16_13	0x0158	/*  Sensor Tx Mapping 13	*/
#define F54_ANALOG_CTRL16_14	0x0159	/*  Sensor Tx Mapping 14	*/
#define F54_ANALOG_CTRL16_15	0x015A	/*  Sensor Tx Mapping 15	*/
#define F54_ANALOG_CTRL16_16	0x015B	/*  Sensor Tx Mapping 16	*/
#define F54_ANALOG_CTRL16_17	0x015C	/*  Sensor Tx Mapping 17	*/
#define F54_ANALOG_CTRL16_18	0x015D	/*  Sensor Tx Mapping 18	*/
#define F54_ANALOG_CTRL16_19	0x015E	/*  Sensor Tx Mapping 19	*/
#define F54_ANALOG_CTRL16_20	0x015F	/*  Sensor Tx Mapping 20	*/
#define F54_ANALOG_CTRL16_21	0x0160	/*  Sensor Tx Mapping 21	*/
#define F54_ANALOG_CTRL16_22	0x0161	/*  Sensor Tx Mapping 22	*/
#define F54_ANALOG_CTRL16_23	0x0162	/*  Sensor Tx Mapping 23	*/
#define F54_ANALOG_CTRL16_24	0x0163	/*  Sensor Tx Mapping 24	*/
#define F54_ANALOG_CTRL16_25	0x0164	/*  Sensor Tx Mapping 25	*/
#define F54_ANALOG_CTRL16_26	0x0165	/*  Sensor Tx Mapping 26	*/
#define F54_ANALOG_CTRL16_27	0x0166	/*  Sensor Tx Mapping 27	*/
#define F54_ANALOG_CTRL16_28	0x0167	/*  Sensor Tx Mapping 28	*/
#define F54_ANALOG_CTRL16_29	0x0168	/*  Sensor Tx Mapping 29	*/
#define F54_ANALOG_CTRL17_00	0x0169	/*  Sense Frequency Control0 0  */
#define F54_ANALOG_CTRL17_01	0x016A	/*  Sense Frequency Control0 1  */
#define F54_ANALOG_CTRL17_02	0x016B	/*  Sense Frequency Control0 2  */
#define F54_ANALOG_CTRL17_03	0x016C	/*  Sense Frequency Control0 3  */
#define F54_ANALOG_CTRL17_04	0x016D	/*  Sense Frequency Control0 4  */
#define F54_ANALOG_CTRL17_05	0x016E	/*  Sense Frequency Control0 5  */
#define F54_ANALOG_CTRL17_06	0x016F	/*  Sense Frequency Control0 6  */
#define F54_ANALOG_CTRL17_07	0x0170	/*  Sense Frequency Control0 7  */
#define F54_ANALOG_CTRL18_00	0x0171	/*  Sense Frequency Control Low 0 */
#define F54_ANALOG_CTRL18_01	0x0172	/*  Sense Frequency Control Low 1 */
#define F54_ANALOG_CTRL18_02	0x0173	/*  Sense Frequency Control Low 2 */
#define F54_ANALOG_CTRL18_03	0x0174	/*  Sense Frequency Control Low 3 */
#define F54_ANALOG_CTRL18_04	0x0175	/*  Sense Frequency Control Low 4 */
#define F54_ANALOG_CTRL18_05	0x0176	/*  Sense Frequency Control Low 5 */
#define F54_ANALOG_CTRL18_06	0x0177	/*  Sense Frequency Control Low 6 */
#define F54_ANALOG_CTRL18_07	0x0178	/*  Sense Frequency Control Low 7 */
#define F54_ANALOG_CTRL19_00	0x0179	/*  Sense Frequency Control2 0	*/
#define F54_ANALOG_CTRL19_01	0x017A	/*  Sense Frequency Control2 1	*/
#define F54_ANALOG_CTRL19_02	0x017B	/*  Sense Frequency Control2 2	*/
#define F54_ANALOG_CTRL19_03	0x017C	/*  Sense Frequency Control2 3	*/
#define F54_ANALOG_CTRL19_04	0x017D	/*  Sense Frequency Control2 4	*/
#define F54_ANALOG_CTRL19_05	0x017E	/*  Sense Frequency Control2 5	*/
#define F54_ANALOG_CTRL19_06	0x017F	/*  Sense Frequency Control2 6	*/
#define F54_ANALOG_CTRL19_07	0x0180	/*  Sense Frequency Control2 7	*/
#define F54_ANALOG_CTRL20	0x0181	/*  Noise Mitigation General Control */
#define F54_ANALOG_CTRL21_00	0x0182	/*  HNM Frequency
						Shift Noise Threshold Low  */
#define F54_ANALOG_CTRL21_01	0x0183	/*  HNM Frequency
						Shift Noise Threshold High */
#define F54_ANALOG_CTRL22	0x0184	/*  HNM Exit Density		*/
#define F54_ANALOG_CTRL23_00	0x0185	/*  Medium Noise Threshold Low  */
#define F54_ANALOG_CTRL23_01	0x0186	/*  Medium Noise Threshold High */
#define F54_ANALOG_CTRL24_00	0x0187	/*  High Noise Threshold Low    */
#define F54_ANALOG_CTRL24_01	0x0188	/*  High Noise Threshold High   */
#define F54_ANALOG_CTRL25	0x0189	/*  FNM Frequency Shift Density */
#define F54_ANALOG_CTRL26	0x018A	/*  FNM Exit Threshold		*/
#define F54_ANALOG_CTRL27	0x018B	/*  IIR Filter Coefficient	*/
#define F54_ANALOG_CTRL28_00	0x018C	/*  FNM Frequency
						Shift Noise Threshold Low  */
#define F54_ANALOG_CTRL28_01	0x018D	/*  FNM Frequency
						Shift Noise Threshold High */
#define F54_ANALOG_CTRL29	0x018E	/*  Common-Mode Noise Control	*/
#define F54_ANALOG_CTRL30	0x018F	/*  CMN Cap Scale Factor	*/
#define F54_ANALOG_CTRL31	0x0190	/*  Pixel Threshold Hysteresis  */
#define F54_ANALOG_CTRL32_00	0x0191	/*  Rx LowEdge Compensation Low */
#define F54_ANALOG_CTRL32_01	0x0192	/*  Rx LowEdge Compensation High*/
#define F54_ANALOG_CTRL33_00	0x0193	/*  Rx HighEdge Compensation Low  */
#define F54_ANALOG_CTRL33_01	0x0194	/*  Rx HighEdge Compensation High */
#define F54_ANALOG_CTRL34_00	0x0195	/*  Tx LowEdge Compensation Low	*/
#define F54_ANALOG_CTRL34_01	0x0196	/*  Tx LowEdge Compensation High */
#define F54_ANALOG_CTRL35_00	0x0197	/*  Tx HighEdge Compensation Low */
#define F54_ANALOG_CTRL35_01	0x0198	/*  Tx HighEdge Compensation High */
#define F54_ANALOG_CTRL38_00	0x0199	/*  Noise Control 1 0  */
#define F54_ANALOG_CTRL38_01	0x019A	/*  Noise Control 1 1  */
#define F54_ANALOG_CTRL38_02	0x019B	/*  Noise Control 1 2  */
#define F54_ANALOG_CTRL38_03	0x019C	/*  Noise Control 1 3  */
#define F54_ANALOG_CTRL38_04	0x019D	/*  Noise Control 1 4  */
#define F54_ANALOG_CTRL38_05	0x019E	/*  Noise Control 1 5  */
#define F54_ANALOG_CTRL38_06	0x019F	/*  Noise Control 1 6  */
#define F54_ANALOG_CTRL38_07	0x01A0	/*  Noise Control 1 7  */
#define F54_ANALOG_CTRL39_00	0x01A1	/*  Noise Control 2 0  */
#define F54_ANALOG_CTRL39_01	0x01A2	/*  Noise Control 2 1  */
#define F54_ANALOG_CTRL39_02	0x01A3	/*  Noise Control 2 2  */
#define F54_ANALOG_CTRL39_03	0x01A4	/*  Noise Control 2 3  */
#define F54_ANALOG_CTRL39_04	0x01A5	/*  Noise Control 2 4  */
#define F54_ANALOG_CTRL39_05	0x01A6	/*  Noise Control 2 5  */
#define F54_ANALOG_CTRL39_06	0x01A7	/*  Noise Control 2 6  */
#define F54_ANALOG_CTRL39_07	0x01A8	/*  Noise Control 2 7  */
#define F54_ANALOG_CTRL40_00	0x01A9	/*  Noise Control 3 0  */
#define F54_ANALOG_CTRL40_01	0x01AA	/*  Noise Control 3 1  */
#define F54_ANALOG_CTRL40_02	0x01AB	/*  Noise Control 3 2  */
#define F54_ANALOG_CTRL40_03	0x01AC	/*  Noise Control 3 3  */
#define F54_ANALOG_CTRL40_04	0x01AD	/*  Noise Control 3 4  */
#define F54_ANALOG_CTRL40_05	0x01AE	/*  Noise Control 3 5  */
#define F54_ANALOG_CTRL40_06	0x01AF	/*  Noise Control 3 6  */
#define F54_ANALOG_CTRL40_07	0x01B0	/*  Noise Control 3 7  */
#define F54_ANALOG_CTRL41	0x01B1	/*  Multi Metric Noise
							Mitigation Control   */
#define F54_ANALOG_CTRL42_00	0x01B2	/*  Burst Span Metric Threshold Low  */
#define F54_ANALOG_CTRL42_01	0x01B3	/*  Burst Span Metric Threshold High */

/**** Register values ****/
struct RegisterValue {
	unsigned short Address;
	unsigned char Value;
};

struct RegisterValue value[] = {
	{F34_FLASH_CTRL00_00, 0x45},	/*  Customer Defined Config ID 0  */
	{F34_FLASH_CTRL00_01, 0x30},	/*  Customer Defined Config ID 1  */
	{F34_FLASH_CTRL00_02, 0x30},	/*  Customer Defined Config ID 2  */
	{F34_FLASH_CTRL00_03, 0x32},	/*  Customer Defined Config ID 3  */
	{F01_RMI_CTRL00, 0x04},	/*  Device Control  */
	{F01_RMI_CTRL01_00, 0x0F},	/*  Interrupt Enable 0        */
	{F01_RMI_CTRL02, 0x03},	/*  Doze Period         */
	{F01_RMI_CTRL03, 0x1E},	/*  Doze Wakeup        Threshold  */
	{F01_RMI_CTRL05, 0x05},	/*  Extended Wait Before Doze  */
	{F01_RMI_CTRL06, 0x20},	/*  I2C        Control         */
	{F01_RMI_CTRL08, 0x41},	/*  Attention Control  */
	{F11_2D_CTRL00, 0x08},	/*  2D Report Mode  */
	{F11_2D_CTRL01, 0x0B},	/*  2D Palm Detect  */
	{F11_2D_CTRL02, 0x19},	/*  2D Delta-X Thresh  */
	{F11_2D_CTRL03, 0x19},	/*  2D Delta-Y Thresh  */
	{F11_2D_CTRL04, 0x00},	/*  2D Velocity         */
	{F11_2D_CTRL05, 0x00},	/*  2D Acceleration  */
	{F11_2D_CTRL06, 0x1F},	/*  2D Max X Position (7:0)  */
	{F11_2D_CTRL07, 0x03},	/*  2D Max X Position (11:8)  */
	{F11_2D_CTRL08, 0xFF},	/*  2D Max Y Position (7:0)  */
	{F11_2D_CTRL09, 0x04},	/*  2D Max Y Position (11:8)  */
	{F11_2D_CTRL29, 0x1E},	/*  Z Touch Threshold  */
	{F11_2D_CTRL30, 0x05},	/*  Z Touch Hysteresis        */
	{F11_2D_CTRL31, 0x2D},	/*  Small Z Threshold  */
	{F11_2D_CTRL32_00, 0x64},	/*  Small Z Scale Factor  */
	{F11_2D_CTRL32_01, 0x09},	/*  Small Z Scale Factor  */
	{F11_2D_CTRL33_00, 0x91},	/*  Large Z Scale Factor  */
	{F11_2D_CTRL33_01, 0x02},	/*  Large Z Scale Factor  */
	{F11_2D_CTRL34, 0x01},	/*  Position Calculation & Post Correction  */
	{F11_2D_CTRL36, 0x41},	/*  Wx Scale Factor  */
	{F11_2D_CTRL37, 0xFE},	/*  Wx Offset  */
	{F11_2D_CTRL38, 0x42},	/*  Wy Scale Factor  */
	{F11_2D_CTRL39, 0xFE},	/*  Wy Offset  */
	{F11_2D_CTRL40_00, 0x33},	/*  X Pitch  */
	{F11_2D_CTRL40_01, 0x53},	/*  X Pitch  */
	{F11_2D_CTRL41_00, 0x1F},	/*  Y Pitch  */
	{F11_2D_CTRL41_01, 0x51},	/*  Y Pitch  */
	{F11_2D_CTRL42_00, 0x39},	/*  Default Finger Width Tx  */
	{F11_2D_CTRL42_01, 0xB8},	/*  Default Finger Width Tx  */
	{F11_2D_CTRL43_00, 0xD7},	/*  Default Finger Width Ty  */
	{F11_2D_CTRL43_01, 0xC1},	/*  Default Finger Width Ty  */
	{F11_2D_CTRL44, 0x00},	/*  Report Finger Width         */
	{F11_2D_CTRL45, 0x70},	/*  Segmentation Aggressiveness     */
	{F11_2D_CTRL46, 0x10},	/*  Rx Clip Low         */
	{F11_2D_CTRL47, 0x10},	/*  Rx Clip High  */
	{F11_2D_CTRL48, 0x10},	/*  Tx Clip Low         */
	{F11_2D_CTRL49, 0x10},	/*  Tx Clip High  */
	{F11_2D_CTRL50, 0x0A},	/*  Minimum Finger Separation  */
	{F11_2D_CTRL51, 0x04},	/*  Maximum Finger Movement  */
	{F11_2D_CTRL58, 0xC0},	/*  Large Object Suppression Parameters */
	{F11_2D_CTRL73, 0x00},	/*  Jitter Control 1  */
	{F11_2D_CTRL74, 0x08},	/*  Jitter Control 2  */
	{F11_2D_CTRL75_00, 0xA2},	/*  Jitter Control 3  */
	{F11_2D_CTRL75_01, 0x09},	/*  Jitter Control 3  */
	{F11_2D_CTRL75_02, 0x28},	/*  Jitter Control 3  */
	{F11_2D_CTRL76_00, 0x06},	/*  Jitter Control 4  */
	{F11_2D_CTRL76_01, 0x0B},	/*  Jitter Control 4  */
	{F11_2D_CTRL76_02, 0x83},	/*  Jitter Control 4  */
	{F54_ANALOG_CTRL00, 0x00},	/*  General Control  */
	{F54_ANALOG_CTRL01, 0x02},	/*  General Control 1        */
	{F54_ANALOG_CTRL02_00, 0x2C},	/*  Saturation Capacitance Low */
	{F54_ANALOG_CTRL02_01, 0x01},	/*  Saturation Capacitance High */
	{F54_ANALOG_CTRL03, 0x80},	/*  Pixel Touch Threshold  */
	{F54_ANALOG_CTRL04, 0x01},	/*  Misc Analog Control         */
	{F54_ANALOG_CTRL05, 0x0E},	/*  RefCap RefLo Settings  */
	{F54_ANALOG_CTRL06, 0x1F},	/*  RefCap RefHi Settings  */
	{F54_ANALOG_CTRL07, 0x13},	/*  CBC        Cap Settings  */
	{F54_ANALOG_CTRL08_00, 0x78},	/*  Integration        Duration Low  */
	{F54_ANALOG_CTRL08_01, 0x00},	/*  Integration        Duration High  */
	{F54_ANALOG_CTRL09, 0x19},	/*  Reset Duration  */
	{F54_ANALOG_CTRL10, 0x04},	/*  Noise Measurement Control  */
	{F54_ANALOG_CTRL11_00, 0x00},	/*  Doze Wakeup        Threshold Low  */
	{F54_ANALOG_CTRL11_01, 0x00},	/*  Doze Wakeup        Threshold High */
	{F54_ANALOG_CTRL12, 0x10},	/*  Slow Relaxation Rate  */
	{F54_ANALOG_CTRL13, 0x0A},	/*  Fast Relaxation Rate  */
	{F54_ANALOG_CTRL14, 0x00},	/*  Sensor Physical Properties        */
	{F54_ANALOG_CTRL15_00, 0x2B},	/*  Sensor Rx Mapping 0         */
	{F54_ANALOG_CTRL15_01, 0x2A},	/*  Sensor Rx Mapping 1         */
	{F54_ANALOG_CTRL15_02, 0x29},	/*  Sensor Rx Mapping 2         */
	{F54_ANALOG_CTRL15_03, 0x27},	/*  Sensor Rx Mapping 3         */
	{F54_ANALOG_CTRL15_04, 0x26},	/*  Sensor Rx Mapping 4         */
	{F54_ANALOG_CTRL15_05, 0x25},	/*  Sensor Rx Mapping 5         */
	{F54_ANALOG_CTRL15_06, 0x24},	/*  Sensor Rx Mapping 6         */
	{F54_ANALOG_CTRL15_07, 0x23},	/*  Sensor Rx Mapping 7         */
	{F54_ANALOG_CTRL15_08, 0x22},	/*  Sensor Rx Mapping 8         */
	{F54_ANALOG_CTRL15_09, 0x21},	/*  Sensor Rx Mapping 9         */
	{F54_ANALOG_CTRL15_10, 0x1F},	/*  Sensor Rx Mapping 10  */
	{F54_ANALOG_CTRL15_11, 0x1E},	/*  Sensor Rx Mapping 11  */
	{F54_ANALOG_CTRL15_12, 0x1D},	/*  Sensor Rx Mapping 12  */
	{F54_ANALOG_CTRL15_13, 0x1C},	/*  Sensor Rx Mapping 13  */
	{F54_ANALOG_CTRL15_14, 0x1B},	/*  Sensor Rx Mapping 14  */
	{F54_ANALOG_CTRL15_15, 0x1A},	/*  Sensor Rx Mapping 15  */
	{F54_ANALOG_CTRL15_16, 0x19},	/*  Sensor Rx Mapping 16  */
	{F54_ANALOG_CTRL15_17, 0x18},	/*  Sensor Rx Mapping 17  */
	{F54_ANALOG_CTRL15_18, 0x17},	/*  Sensor Rx Mapping 18  */
	{F54_ANALOG_CTRL15_19, 0x16},	/*  Sensor Rx Mapping 19  */
	{F54_ANALOG_CTRL15_20, 0x15},	/*  Sensor Rx Mapping 20  */
	{F54_ANALOG_CTRL15_21, 0x14},	/*  Sensor Rx Mapping 21  */
	{F54_ANALOG_CTRL15_22, 0x13},	/*  Sensor Rx Mapping 22  */
	{F54_ANALOG_CTRL15_23, 0x12},	/*  Sensor Rx Mapping 23  */
	{F54_ANALOG_CTRL15_24, 0x11},	/*  Sensor Rx Mapping 24  */
	{F54_ANALOG_CTRL15_25, 0x10},	/*  Sensor Rx Mapping 25  */
	{F54_ANALOG_CTRL15_26, 0x0F},	/*  Sensor Rx Mapping 26  */
	{F54_ANALOG_CTRL15_27, 0x0E},	/*  Sensor Rx Mapping 27  */
	{F54_ANALOG_CTRL15_28, 0x0D},	/*  Sensor Rx Mapping 28  */
	{F54_ANALOG_CTRL15_29, 0x0C},	/*  Sensor Rx Mapping 29  */
	{F54_ANALOG_CTRL15_30, 0x0B},	/*  Sensor Rx Mapping 30  */
	{F54_ANALOG_CTRL15_31, 0x0A},	/*  Sensor Rx Mapping 31  */
	{F54_ANALOG_CTRL15_32, 0x09},	/*  Sensor Rx Mapping 32  */
	{F54_ANALOG_CTRL15_33, 0x08},	/*  Sensor Rx Mapping 33  */
	{F54_ANALOG_CTRL15_34, 0x07},	/*  Sensor Rx Mapping 34  */
	{F54_ANALOG_CTRL15_35, 0x06},	/*  Sensor Rx Mapping 35  */
	{F54_ANALOG_CTRL15_36, 0x05},	/*  Sensor Rx Mapping 36  */
	{F54_ANALOG_CTRL15_37, 0x04},	/*  Sensor Rx Mapping 37  */
	{F54_ANALOG_CTRL15_38, 0x03},	/*  Sensor Rx Mapping 38  */
	{F54_ANALOG_CTRL15_39, 0x02},	/*  Sensor Rx Mapping 39  */
	{F54_ANALOG_CTRL15_40, 0x01},	/*  Sensor Rx Mapping 40  */
	{F54_ANALOG_CTRL15_41, 0x00},	/*  Sensor Rx Mapping 41  */
	{F54_ANALOG_CTRL15_42, 0xFF},	/*  Sensor Rx Mapping 42  */
	{F54_ANALOG_CTRL15_43, 0xFF},	/*  Sensor Rx Mapping 43  */
	{F54_ANALOG_CTRL15_44, 0xFF},	/*  Sensor Rx Mapping 44  */
	{F54_ANALOG_CTRL15_45, 0xFF},	/*  Sensor Rx Mapping 45  */
	{F54_ANALOG_CTRL16_00, 0x1D},	/*  Sensor Tx Mapping 0		*/
	{F54_ANALOG_CTRL16_01, 0x1C},	/*  Sensor Tx Mapping 1         */
	{F54_ANALOG_CTRL16_02, 0x1B},	/*  Sensor Tx Mapping 2         */
	{F54_ANALOG_CTRL16_03, 0x1A},	/*  Sensor Tx Mapping 3         */
	{F54_ANALOG_CTRL16_04, 0x19},	/*  Sensor Tx Mapping 4         */
	{F54_ANALOG_CTRL16_05, 0x18},	/*  Sensor Tx Mapping 5         */
	{F54_ANALOG_CTRL16_06, 0x17},	/*  Sensor Tx Mapping 6         */
	{F54_ANALOG_CTRL16_07, 0x16},	/*  Sensor Tx Mapping 7         */
	{F54_ANALOG_CTRL16_08, 0x14},	/*  Sensor Tx Mapping 8         */
	{F54_ANALOG_CTRL16_09, 0x12},	/*  Sensor Tx Mapping 9         */
	{F54_ANALOG_CTRL16_10, 0x10},	/*  Sensor Tx Mapping 10  */
	{F54_ANALOG_CTRL16_11, 0x0F},	/*  Sensor Tx Mapping 11  */
	{F54_ANALOG_CTRL16_12, 0x0E},	/*  Sensor Tx Mapping 12  */
	{F54_ANALOG_CTRL16_13, 0x0D},	/*  Sensor Tx Mapping 13  */
	{F54_ANALOG_CTRL16_14, 0x0C},	/*  Sensor Tx Mapping 14  */
	{F54_ANALOG_CTRL16_15, 0x0B},	/*  Sensor Tx Mapping 15  */
	{F54_ANALOG_CTRL16_16, 0x0A},	/*  Sensor Tx Mapping 16  */
	{F54_ANALOG_CTRL16_17, 0x09},	/*  Sensor Tx Mapping 17  */
	{F54_ANALOG_CTRL16_18, 0x08},	/*  Sensor Tx Mapping 18  */
	{F54_ANALOG_CTRL16_19, 0x07},	/*  Sensor Tx Mapping 19  */
	{F54_ANALOG_CTRL16_20, 0x06},	/*  Sensor Tx Mapping 20  */
	{F54_ANALOG_CTRL16_21, 0x05},	/*  Sensor Tx Mapping 21  */
	{F54_ANALOG_CTRL16_22, 0x04},	/*  Sensor Tx Mapping 22  */
	{F54_ANALOG_CTRL16_23, 0x03},	/*  Sensor Tx Mapping 23  */
	{F54_ANALOG_CTRL16_24, 0x02},	/*  Sensor Tx Mapping 24  */
	{F54_ANALOG_CTRL16_25, 0x01},	/*  Sensor Tx Mapping 25  */
	{F54_ANALOG_CTRL16_26, 0x00},	/*  Sensor Tx Mapping 26  */
	{F54_ANALOG_CTRL16_27, 0xFF},	/*  Sensor Tx Mapping 27  */
	{F54_ANALOG_CTRL16_28, 0xFF},	/*  Sensor Tx Mapping 28  */
	{F54_ANALOG_CTRL16_29, 0xFF},	/*  Sensor Tx Mapping 29  */
	{F54_ANALOG_CTRL17_00, 0x60},	/*  Sense Frequency Control0 0        */
	{F54_ANALOG_CTRL17_01, 0x60},	/*  Sense Frequency Control0 1        */
	{F54_ANALOG_CTRL17_02, 0x40},	/*  Sense Frequency Control0 2        */
	{F54_ANALOG_CTRL17_03, 0x40},	/*  Sense Frequency Control0 3        */
	{F54_ANALOG_CTRL17_04, 0x40},	/*  Sense Frequency Control0 4        */
	{F54_ANALOG_CTRL17_05, 0x40},	/*  Sense Frequency Control0 5        */
	{F54_ANALOG_CTRL17_06, 0x40},	/*  Sense Frequency Control0 6        */
	{F54_ANALOG_CTRL17_07, 0x20},	/*  Sense Frequency Control0 7        */
	{F54_ANALOG_CTRL18_00, 0x1E},	/*  Sense Frequency Control Low 0  */
	{F54_ANALOG_CTRL18_01, 0x1D},	/*  Sense Frequency Control Low 1  */
	{F54_ANALOG_CTRL18_02, 0x1C},	/*  Sense Frequency Control Low 2  */
	{F54_ANALOG_CTRL18_03, 0x1B},	/*  Sense Frequency Control Low 3  */
	{F54_ANALOG_CTRL18_04, 0x1A},	/*  Sense Frequency Control Low 4  */
	{F54_ANALOG_CTRL18_05, 0x19},	/*  Sense Frequency Control Low 5  */
	{F54_ANALOG_CTRL18_06, 0x18},	/*  Sense Frequency Control Low 6  */
	{F54_ANALOG_CTRL18_07, 0x17},	/*  Sense Frequency Control Low 7  */
	{F54_ANALOG_CTRL19_00, 0x01},	/*  Sense Frequency Control2 0        */
	{F54_ANALOG_CTRL19_01, 0x07},	/*  Sense Frequency Control2 1        */
	{F54_ANALOG_CTRL19_02, 0x0D},	/*  Sense Frequency Control2 2        */
	{F54_ANALOG_CTRL19_03, 0x15},	/*  Sense Frequency Control2 3        */
	{F54_ANALOG_CTRL19_04, 0x1C},	/*  Sense Frequency Control2 4        */
	{F54_ANALOG_CTRL19_05, 0x25},	/*  Sense Frequency Control2 5        */
	{F54_ANALOG_CTRL19_06, 0x2D},	/*  Sense Frequency Control2 6        */
	{F54_ANALOG_CTRL19_07, 0x36},	/*  Sense Frequency Control2 7        */
	{F54_ANALOG_CTRL20, 0x00},	/*  Noise Mitigation General Control  */
	{F54_ANALOG_CTRL21_00, 0xE8},	/*  HNM Fequency
						Shift Noise Threshold Low  */
	{F54_ANALOG_CTRL21_01, 0x03},	/*  HNM Frequency
						Shift Noise Threshold High  */
	{F54_ANALOG_CTRL22, 0xFF},	/*  HNM Exit Density  */
	{F54_ANALOG_CTRL23_00, 0xD0},	/*  Medium Noise Threshold Low        */
	{F54_ANALOG_CTRL23_01, 0x07},	/*  Medium Noise Threshold High       */
	{F54_ANALOG_CTRL24_00, 0xC8},	/*  High Noise Threshold Low  */
	{F54_ANALOG_CTRL24_01, 0x00},	/*  High Noise Threshold High  */
	{F54_ANALOG_CTRL25, 0xB3},	/*  FNM Frequency Shift Density       */
	{F54_ANALOG_CTRL26, 0x32},	/*  FNM Exit Threshold        */
	{F54_ANALOG_CTRL27, 0x80},	/*  IIR Filter Coefficient  */
	{F54_ANALOG_CTRL28_00, 0xB8},	/*  FNM Frequency
						Shift Noise Threshold Low  */
	{F54_ANALOG_CTRL28_01, 0x0B},	/*  FNM Frequency
						Shift Noise Threshold High  */
	{F54_ANALOG_CTRL29, 0x00},	/*  Common-Mode Noise Control */
	{F54_ANALOG_CTRL30, 0xC0},	/*  CMN Cap Scale Factor  */
	{F54_ANALOG_CTRL31, 0x80},	/*  Pixel Threshold Hysteresis */
	{F54_ANALOG_CTRL32_00, 0x00},	/*  Rx LowEdge Compensation Low */
	{F54_ANALOG_CTRL32_01, 0x10},	/*  Rx LowEdge Compensation High */
	{F54_ANALOG_CTRL33_00, 0x00},	/*  Rx HighEdge Compensation Low */
	{F54_ANALOG_CTRL33_01, 0x10},	/*  Rx HighEdge Compensation High */
	{F54_ANALOG_CTRL34_00, 0x00},	/*  Tx LowEdge Compensation Low */
	{F54_ANALOG_CTRL34_01, 0x10},	/*  Tx LowEdge Compensation High */
	{F54_ANALOG_CTRL35_00, 0x00},	/*  Tx HighEdge Compensation Low */
	{F54_ANALOG_CTRL35_01, 0x10},	/*  Tx HighEdge Compensation High */
	{F54_ANALOG_CTRL38_00, 0x07},	/*  Noise Control 1 0  */
	{F54_ANALOG_CTRL38_01, 0x02},	/*  Noise Control 1 1  */
	{F54_ANALOG_CTRL38_02, 0x02},	/*  Noise Control 1 2  */
	{F54_ANALOG_CTRL38_03, 0x02},	/*  Noise Control 1 3  */
	{F54_ANALOG_CTRL38_04, 0x07},	/*  Noise Control 1 4  */
	{F54_ANALOG_CTRL38_05, 0x02},	/*  Noise Control 1 5  */
	{F54_ANALOG_CTRL38_06, 0x02},	/*  Noise Control 1 6  */
	{F54_ANALOG_CTRL38_07, 0x05},	/*  Noise Control 1 7  */
	{F54_ANALOG_CTRL39_00, 0x40},	/*  Noise Control 2 0  */
	{F54_ANALOG_CTRL39_01, 0x10},	/*  Noise Control 2 1  */
	{F54_ANALOG_CTRL39_02, 0x10},	/*  Noise Control 2 2  */
	{F54_ANALOG_CTRL39_03, 0x10},	/*  Noise Control 2 3  */
	{F54_ANALOG_CTRL39_04, 0x40},	/*  Noise Control 2 4  */
	{F54_ANALOG_CTRL39_05, 0x10},	/*  Noise Control 2 5  */
	{F54_ANALOG_CTRL39_06, 0x10},	/*  Noise Control 2 6  */
	{F54_ANALOG_CTRL39_07, 0x20},	/*  Noise Control 2 7  */
	{F54_ANALOG_CTRL40_00, 0x5C},	/*  Noise Control 3 0  */
	{F54_ANALOG_CTRL40_01, 0x54},	/*  Noise Control 3 1  */
	{F54_ANALOG_CTRL40_02, 0x57},	/*  Noise Control 3 2  */
	{F54_ANALOG_CTRL40_03, 0x5B},	/*  Noise Control 3 3  */
	{F54_ANALOG_CTRL40_04, 0x6C},	/*  Noise Control 3 4  */
	{F54_ANALOG_CTRL40_05, 0x63},	/*  Noise Control 3 5  */
	{F54_ANALOG_CTRL40_06, 0x67},	/*  Noise Control 3 6  */
	{F54_ANALOG_CTRL40_07, 0x56},	/*  Noise Control 3 7  */
	{F54_ANALOG_CTRL41, 0x00},	/*  Multi Metric Noise
							Mitigation Control  */
	{F54_ANALOG_CTRL42_00, 0x64},	/*  Burst Span Metric Threshold Low  */
	{F54_ANALOG_CTRL42_01, 0x00}	/*  Burst Span Metric Threshold High  */
};

/**** Lockdown data ****/
unsigned char lockdown[] = { 0xD4, 0x62, 0x53, 0x0A, 0x12, 0x3F, 0x8F, 0x57,
	0x8C, 0x14, 0xA3, 0x0F, 0x89, 0xF1, 0x30, 0x51, 0x63, 0xF3, 0x31, 0x53,
	0xA7, 0x28, 0xD4, 0x67, 0x2E, 0xBF, 0xDC, 0x14, 0x24, 0xA0, 0x02, 0x8A,
	0x34, 0xA1, 0x22, 0xF9, 0x23, 0xD3, 0xD6, 0x28, 0xDC, 0x2B, 0xF2, 0x0B,
	0xE7, 0x02, 0xA1, 0x24, 0xA7, 0x70, 0x25, 0x29, 0x7C, 0x2E, 0x27, 0x08,
	0x51, 0x77, 0x9B, 0x84, 0x33, 0xE5, 0x9E, 0x0F, 0xA0, 0x01, 0x21, 0xAA,
	0x69, 0xA9, 0x43, 0x47, 0xD8, 0x6C, 0x58, 0x99, 0x75, 0x53, 0x39, 0x5E
};

/**** Protocol info ****/
#define AE_LOW				0
#define AE_HIGH				1
#define ATTENTION			AE_LOW
#define PROTOCOL_I2C			1
#define PROTOCOL_I2C_SLAVE_ADDR		0x20
