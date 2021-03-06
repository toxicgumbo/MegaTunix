 
; Generated 05/07/2004 22:21:41 by MStoolsII 
; GM #12146312 
; Standard GM type Coolant Thermistor for Megasquirt 
; Computed Steinhart-Hart coefficients: A=0.00162549  B=0.00019531  C=3.00954e-07
;	 0 Fahrenheit = 25000 ohm
;	 100 Fahrenheit =  1800 ohm
;	 210 Fahrenheit =   185 ohm
; For use with  2490 ohm bias resistor installed at R7.
THERMFACTOR:
		;	 ADC 	 Fahrenheit - Celcius - resistance in ohms
	DB	210T;	   0 - sensor failure, use limp home value.
	DB	255T;	   1	  408 	 209 	     10 ohms
	DB	255T;	   2	  353 	 178 	     20 ohms
	DB	255T;	   3	  323 	 162 	     30 ohms
	DB	255T;	   4	  303 	 151 	     40 ohms
	DB	255T;	   5	  288 	 142 	     50 ohms
	DB	255T;	   6	  276 	 136 	     60 ohms
	DB	255T;	   7	  266 	 130 	     70 ohms
	DB	255T;	   8	  258 	 126 	     81 ohms
	DB	255T;	   9	  251 	 121 	     91 ohms
	DB	255T;	  10	  244 	 118 	    102 ohms
	DB	255T;	  11	  238 	 115 	    112 ohms
	DB	255T;	  12	  233 	 112 	    123 ohms
	DB	255T;	  13	  228 	 109 	    134 ohms
	DB	255T;	  14	  224 	 106 	    145 ohms
	DB	255T;	  15	  220 	 104 	    156 ohms
	DB	255T;	  16	  216 	 102 	    167 ohms
	DB	252T;	  17	  212 	 100 	    178 ohms
	DB	248T;	  18	  209 	  98 	    189 ohms
	DB	245T;	  19	  206 	  96 	    200 ohms
	DB	242T;	  20	  203 	  95 	    212 ohms
	DB	239T;	  21	  200 	  93 	    223 ohms
	DB	237T;	  22	  197 	  92 	    235 ohms
	DB	234T;	  23	  194 	  90 	    247 ohms
	DB	231T;	  24	  192 	  89 	    259 ohms
	DB	229T;	  25	  190 	  88 	    271 ohms
	DB	227T;	  26	  187 	  86 	    283 ohms
	DB	225T;	  27	  185 	  85 	    295 ohms
	DB	223T;	  28	  183 	  84 	    307 ohms
	DB	221T;	  29	  181 	  83 	    320 ohms
	DB	219T;	  30	  179 	  82 	    332 ohms
	DB	217T;	  31	  177 	  81 	    345 ohms
	DB	215T;	  32	  175 	  80 	    357 ohms
	DB	213T;	  33	  174 	  79 	    370 ohms
	DB	211T;	  34	  172 	  78 	    383 ohms
	DB	210T;	  35	  170 	  77 	    396 ohms
	DB	208T;	  36	  168 	  76 	    409 ohms
	DB	206T;	  37	  167 	  75 	    423 ohms
	DB	205T;	  38	  165 	  74 	    436 ohms
	DB	203T;	  39	  164 	  73 	    450 ohms
	DB	202T;	  40	  162 	  72 	    463 ohms
	DB	200T;	  41	  161 	  72 	    477 ohms
	DB	199T;	  42	  159 	  71 	    491 ohms
	DB	198T;	  43	  158 	  70 	    505 ohms
	DB	196T;	  44	  157 	  69 	    519 ohms
	DB	195T;	  45	  155 	  69 	    534 ohms
	DB	194T;	  46	  154 	  68 	    548 ohms
	DB	192T;	  47	  153 	  67 	    563 ohms
	DB	191T;	  48	  152 	  66 	    577 ohms
	DB	190T;	  49	  150 	  66 	    592 ohms
	DB	189T;	  50	  149 	  65 	    607 ohms
	DB	187T;	  51	  148 	  64 	    622 ohms
	DB	186T;	  52	  147 	  64 	    638 ohms
	DB	185T;	  53	  146 	  63 	    653 ohms
	DB	184T;	  54	  145 	  63 	    669 ohms
	DB	183T;	  55	  143 	  62 	    685 ohms
	DB	182T;	  56	  142 	  61 	    701 ohms
	DB	181T;	  57	  141 	  61 	    717 ohms
	DB	180T;	  58	  140 	  60 	    733 ohms
	DB	179T;	  59	  139 	  60 	    750 ohms
	DB	178T;	  60	  138 	  59 	    766 ohms
	DB	177T;	  61	  137 	  58 	    783 ohms
	DB	176T;	  62	  136 	  58 	    800 ohms
	DB	175T;	  63	  135 	  57 	    817 ohms
	DB	174T;	  64	  134 	  57 	    834 ohms
	DB	173T;	  65	  133 	  56 	    852 ohms
	DB	172T;	  66	  132 	  56 	    870 ohms
	DB	171T;	  67	  131 	  55 	    887 ohms
	DB	170T;	  68	  130 	  55 	    905 ohms
	DB	169T;	  69	  130 	  54 	    924 ohms
	DB	168T;	  70	  129 	  54 	    942 ohms
	DB	167T;	  71	  128 	  53 	    961 ohms
	DB	166T;	  72	  127 	  53 	    980 ohms
	DB	165T;	  73	  126 	  52 	    999 ohms
	DB	165T;	  74	  125 	  52 	   1018 ohms
	DB	164T;	  75	  124 	  51 	   1038 ohms
	DB	163T;	  76	  123 	  51 	   1057 ohms
	DB	162T;	  77	  123 	  50 	   1077 ohms
	DB	161T;	  78	  122 	  50 	   1097 ohms
	DB	160T;	  79	  121 	  49 	   1118 ohms
	DB	160T;	  80	  120 	  49 	   1138 ohms
	DB	159T;	  81	  119 	  48 	   1159 ohms
	DB	158T;	  82	  118 	  48 	   1180 ohms
	DB	157T;	  83	  118 	  48 	   1202 ohms
	DB	156T;	  84	  117 	  47 	   1223 ohms
	DB	156T;	  85	  116 	  47 	   1245 ohms
	DB	155T;	  86	  115 	  46 	   1267 ohms
	DB	154T;	  87	  115 	  46 	   1289 ohms
	DB	153T;	  88	  114 	  45 	   1312 ohms
	DB	152T;	  89	  113 	  45 	   1335 ohms
	DB	152T;	  90	  112 	  45 	   1358 ohms
	DB	151T;	  91	  111 	  44 	   1382 ohms
	DB	150T;	  92	  111 	  44 	   1405 ohms
	DB	149T;	  93	  110 	  43 	   1429 ohms
	DB	149T;	  94	  109 	  43 	   1454 ohms
	DB	148T;	  95	  109 	  43 	   1478 ohms
	DB	147T;	  96	  108 	  42 	   1503 ohms
	DB	147T;	  97	  107 	  42 	   1529 ohms
	DB	146T;	  98	  106 	  41 	   1554 ohms
	DB	145T;	  99	  106 	  41 	   1580 ohms
	DB	144T;	 100	  105 	  40 	   1606 ohms
	DB	144T;	 101	  104 	  40 	   1633 ohms
	DB	143T;	 102	  103 	  40 	   1660 ohms
	DB	142T;	 103	  103 	  39 	   1687 ohms
	DB	142T;	 104	  102 	  39 	   1715 ohms
	DB	141T;	 105	  101 	  39 	   1743 ohms
	DB	140T;	 106	  101 	  38 	   1771 ohms
	DB	139T;	 107	  100 	  38 	   1800 ohms
	DB	139T;	 108	   99 	  37 	   1829 ohms
	DB	138T;	 109	   99 	  37 	   1859 ohms
	DB	137T;	 110	   98 	  37 	   1889 ohms
	DB	137T;	 111	   97 	  36 	   1919 ohms
	DB	136T;	 112	   97 	  36 	   1950 ohms
	DB	135T;	 113	   96 	  36 	   1981 ohms
	DB	135T;	 114	   95 	  35 	   2013 ohms
	DB	134T;	 115	   95 	  35 	   2045 ohms
	DB	133T;	 116	   94 	  34 	   2078 ohms
	DB	133T;	 117	   93 	  34 	   2111 ohms
	DB	132T;	 118	   93 	  34 	   2145 ohms
	DB	131T;	 119	   92 	  33 	   2179 ohms
	DB	131T;	 120	   91 	  33 	   2213 ohms
	DB	130T;	 121	   91 	  33 	   2248 ohms
	DB	129T;	 122	   90 	  32 	   2284 ohms
	DB	129T;	 123	   89 	  32 	   2320 ohms
	DB	128T;	 124	   89 	  31 	   2357 ohms
	DB	127T;	 125	   88 	  31 	   2394 ohms
	DB	127T;	 126	   87 	  31 	   2432 ohms
	DB	126T;	 127	   87 	  30 	   2471 ohms
	DB	125T;	 128	   86 	  30 	   2510 ohms
	DB	125T;	 129	   85 	  30 	   2549 ohms
	DB	124T;	 130	   85 	  29 	   2590 ohms
	DB	124T;	 131	   84 	  29 	   2631 ohms
	DB	123T;	 132	   83 	  29 	   2672 ohms
	DB	122T;	 133	   83 	  28 	   2715 ohms
	DB	122T;	 134	   82 	  28 	   2758 ohms
	DB	121T;	 135	   81 	  27 	   2801 ohms
	DB	120T;	 136	   81 	  27 	   2846 ohms
	DB	120T;	 137	   80 	  27 	   2891 ohms
	DB	119T;	 138	   80 	  26 	   2937 ohms
	DB	118T;	 139	   79 	  26 	   2984 ohms
	DB	118T;	 140	   78 	  26 	   3031 ohms
	DB	117T;	 141	   78 	  25 	   3080 ohms
	DB	116T;	 142	   77 	  25 	   3129 ohms
	DB	116T;	 143	   76 	  25 	   3179 ohms
	DB	115T;	 144	   76 	  24 	   3230 ohms
	DB	115T;	 145	   75 	  24 	   3282 ohms
	DB	114T;	 146	   74 	  24 	   3335 ohms
	DB	113T;	 147	   74 	  23 	   3389 ohms
	DB	113T;	 148	   73 	  23 	   3444 ohms
	DB	112T;	 149	   72 	  22 	   3500 ohms
	DB	111T;	 150	   72 	  22 	   3557 ohms
	DB	111T;	 151	   71 	  22 	   3615 ohms
	DB	110T;	 152	   70 	  21 	   3675 ohms
	DB	109T;	 153	   70 	  21 	   3735 ohms
	DB	109T;	 154	   69 	  21 	   3797 ohms
	DB	108T;	 155	   69 	  20 	   3860 ohms
	DB	107T;	 156	   68 	  20 	   3924 ohms
	DB	107T;	 157	   67 	  20 	   3989 ohms
	DB	106T;	 158	   67 	  19 	   4056 ohms
	DB	105T;	 159	   66 	  19 	   4124 ohms
	DB	105T;	 160	   65 	  18 	   4194 ohms
	DB	104T;	 161	   65 	  18 	   4265 ohms
	DB	103T;	 162	   64 	  18 	   4337 ohms
	DB	103T;	 163	   63 	  17 	   4412 ohms
	DB	102T;	 164	   63 	  17 	   4487 ohms
	DB	101T;	 165	   62 	  17 	   4565 ohms
	DB	101T;	 166	   61 	  16 	   4644 ohms
	DB	100T;	 167	   61 	  16 	   4725 ohms
	DB	 99T;	 168	   60 	  15 	   4808 ohms
	DB	 99T;	 169	   59 	  15 	   4893 ohms
	DB	 98T;	 170	   58 	  15 	   4980 ohms
	DB	 97T;	 171	   58 	  14 	   5069 ohms
	DB	 97T;	 172	   57 	  14 	   5160 ohms
	DB	 96T;	 173	   56 	  14 	   5253 ohms
	DB	 95T;	 174	   56 	  13 	   5349 ohms
	DB	 95T;	 175	   55 	  13 	   5447 ohms
	DB	 94T;	 176	   54 	  12 	   5547 ohms
	DB	 93T;	 177	   54 	  12 	   5650 ohms
	DB	 92T;	 178	   53 	  12 	   5756 ohms
	DB	 92T;	 179	   52 	  11 	   5865 ohms
	DB	 91T;	 180	   51 	  11 	   5976 ohms
	DB	 90T;	 181	   51 	  10 	   6090 ohms
	DB	 89T;	 182	   50 	  10 	   6208 ohms
	DB	 89T;	 183	   49 	  10 	   6329 ohms
	DB	 88T;	 184	   49 	   9 	   6453 ohms
	DB	 87T;	 185	   48 	   9 	   6581 ohms
	DB	 87T;	 186	   47 	   8 	   6712 ohms
	DB	 86T;	 187	   46 	   8 	   6848 ohms
	DB	 85T;	 188	   46 	   8 	   6987 ohms
	DB	 84T;	 189	   45 	   7 	   7130 ohms
	DB	 83T;	 190	   44 	   7 	   7278 ohms
	DB	 83T;	 191	   43 	   6 	   7431 ohms
	DB	 82T;	 192	   42 	   6 	   7589 ohms
	DB	 81T;	 193	   42 	   5 	   7751 ohms
	DB	 80T;	 194	   41 	   5 	   7919 ohms
	DB	 80T;	 195	   40 	   4 	   8092 ohms
	DB	 79T;	 196	   39 	   4 	   8272 ohms
	DB	 78T;	 197	   38 	   4 	   8457 ohms
	DB	 77T;	 198	   38 	   3 	   8649 ohms
	DB	 76T;	 199	   37 	   3 	   8848 ohms
	DB	 75T;	 200	   36 	   2 	   9055 ohms
	DB	 74T;	 201	   35 	   2 	   9268 ohms
	DB	 74T;	 202	   34 	   1 	   9490 ohms
	DB	 73T;	 203	   33 	   1 	   9721 ohms
	DB	 72T;	 204	   32 	   0 	   9960 ohms
	DB	 71T;	 205	   31 	  -0 	  10209 ohms
	DB	 70T;	 206	   31 	  -1 	  10468 ohms
	DB	 69T;	 207	   30 	  -1 	  10738 ohms
	DB	 68T;	 208	   29 	  -2 	  11020 ohms
	DB	 67T;	 209	   28 	  -2 	  11313 ohms
	DB	 66T;	 210	   27 	  -3 	  11620 ohms
	DB	 65T;	 211	   26 	  -3 	  11941 ohms
	DB	 64T;	 212	   25 	  -4 	  12276 ohms
	DB	 63T;	 213	   24 	  -5 	  12628 ohms
	DB	 62T;	 214	   23 	  -5 	  12997 ohms
	DB	 61T;	 215	   22 	  -6 	  13384 ohms
	DB	 60T;	 216	   21 	  -6 	  13791 ohms
	DB	 59T;	 217	   20 	  -7 	  14219 ohms
	DB	 58T;	 218	   18 	  -8 	  14671 ohms
	DB	 57T;	 219	   17 	  -8 	  15148 ohms
	DB	 56T;	 220	   16 	  -9 	  15651 ohms
	DB	 54T;	 221	   15 	  -9 	  16185 ohms
	DB	 53T;	 222	   14 	 -10 	  16751 ohms
	DB	 52T;	 223	   13 	 -11 	  17352 ohms
	DB	 51T;	 224	   11 	 -12 	  17992 ohms
	DB	 49T;	 225	   10 	 -12 	  18675 ohms
	DB	 48T;	 226	    9 	 -13 	  19405 ohms
	DB	 47T;	 227	    7 	 -14 	  20187 ohms
	DB	 45T;	 228	    6 	 -15 	  21027 ohms
	DB	 44T;	 229	    4 	 -15 	  21931 ohms
	DB	 42T;	 230	    3 	 -16 	  22908 ohms
	DB	 41T;	 231	    1 	 -17 	  23966 ohms
	DB	 39T;	 232	   -0 	 -18 	  25117 ohms
	DB	 38T;	 233	   -2 	 -19 	  26371 ohms
	DB	 36T;	 234	   -4 	 -20 	  27746 ohms
	DB	 34T;	 235	   -5 	 -21 	  29258 ohms
	DB	 32T;	 236	   -7 	 -22 	  30928 ohms
	DB	 30T;	 237	   -9 	 -23 	  32785 ohms
	DB	 28T;	 238	  -11 	 -24 	  34860 ohms
	DB	 26T;	 239	  -13 	 -25 	  37194 ohms
	DB	 24T;	 240	  -15 	 -26 	  39840 ohms
	DB	 22T;	 241	  -18 	 -28 	  42864 ohms
	DB	 19T;	 242	  -20 	 -29 	  46352 ohms
	DB	 16T;	 243	  -23 	 -31 	  50422 ohms
	DB	 14T;	 244	  -26 	 -32 	  55233 ohms
	DB	 10T;	 245	  -29 	 -34 	  61005 ohms
	DB	  7T;	 246	  -32 	 -36 	  68060 ohms
	DB	  3T;	 247	  -36 	 -38 	  76879 ohms
	DB	  0T;	 248	  -40 	 -40 	  88217 ohms
	DB	  0T;	 249	  -45 	 -43 	 103335 ohms
	DB	  0T;	 250	  -51 	 -46 	 124500 ohms
	DB	  0T;	 251	  -58 	 -50 	 156248 ohms
	DB	  0T;	 252	  -66 	 -54 	 209160 ohms
	DB	  0T;	 253	  -77 	 -61 	 314985 ohms
	DB	  0T;	 254	  -96 	 -71 	 632460 ohms
	DB	210T;	 255 - sensor failure, use limp home value.

