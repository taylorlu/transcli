#include "AudioFilter.h"
#include <math.h>

#define M 15

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028842
#endif

#define RANDBUFLEN 65536

#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))

#ifdef WIN32
extern "C" {_declspec(dllimport) int _stdcall MulDiv(int nNumber,int nNumerator,int nDenominator);}
#else
#define MulDiv(x,y,z) ((x)*(y)/(z))
#endif

extern "C" {
	extern void rdft(int, int, REAL *, int *, REAL *);
}

static double dbesi0(double x)
{
	int k;
	double w, t, y;
	static double a[65] = {
		8.5246820682016865877e-11, 2.5966600546497407288e-9, 
		7.9689994568640180274e-8, 1.9906710409667748239e-6, 
		4.0312469446528002532e-5, 6.4499871606224265421e-4, 
		0.0079012345761930579108, 0.071111111109207045212, 
		0.444444444444724909, 1.7777777777777532045, 
		4.0000000000000011182, 3.99999999999999998, 
		1.0000000000000000001, 
		1.1520919130377195927e-10, 2.2287613013610985225e-9, 
		8.1903951930694585113e-8, 1.9821560631611544984e-6, 
		4.0335461940910133184e-5, 6.4495330974432203401e-4, 
		0.0079013012611467520626, 0.071111038160875566622, 
		0.44444450319062699316, 1.7777777439146450067, 
		4.0000000132337935071, 3.9999999968569015366, 
		1.0000000003426703174, 
		1.5476870780515238488e-10, 1.2685004214732975355e-9, 
		9.2776861851114223267e-8, 1.9063070109379044378e-6, 
		4.0698004389917945832e-5, 6.4370447244298070713e-4, 
		0.0079044749458444976958, 0.071105052411749363882, 
		0.44445280640924755082, 1.7777694934432109713, 
		4.0000055808824003386, 3.9999977081165740932, 
		1.0000004333949319118, 
		2.0675200625006793075e-10, -6.1689554705125681442e-10, 
		1.2436765915401571654e-7, 1.5830429403520613423e-6, 
		4.2947227560776583326e-5, 6.3249861665073441312e-4, 
		0.0079454472840953930811, 0.070994327785661860575, 
		0.44467219586283000332, 1.7774588182255374745, 
		4.0003038986252717972, 3.9998233869142057195, 
		1.0000472932961288324, 
		2.7475684794982708655e-10, -3.8991472076521332023e-9, 
		1.9730170483976049388e-7, 5.9651531561967674521e-7, 
		5.1992971474748995357e-5, 5.7327338675433770752e-4, 
		0.0082293143836530412024, 0.069990934858728039037, 
		0.44726764292723985087, 1.7726685170014087784, 
		4.0062907863712704432, 3.9952750700487845355, 
		1.0016354346654179322
	};
	static double b[70] = {
		6.7852367144945531383e-8, 4.6266061382821826854e-7, 
		6.9703135812354071774e-6, 7.6637663462953234134e-5, 
		7.9113515222612691636e-4, 0.0073401204731103808981, 
		0.060677114958668837046, 0.43994941411651569622, 
		2.7420017097661750609, 14.289661921740860534, 
		59.820609640320710779, 188.78998681199150629, 
		399.8731367825601118, 427.56411572180478514, 
		1.8042097874891098754e-7, 1.2277164312044637357e-6, 
		1.8484393221474274861e-5, 2.0293995900091309208e-4, 
		0.0020918539850246207459, 0.019375315654033949297, 
		0.15985869016767185908, 1.1565260527420641724, 
		7.1896341224206072113, 37.354773811947484532, 
		155.80993164266268457, 489.5211371158540918, 
		1030.9147225169564806, 1093.5883545113746958, 
		4.8017305613187493564e-7, 3.261317843912380074e-6, 
		4.9073137508166159639e-5, 5.3806506676487583755e-4, 
		0.0055387918291051866561, 0.051223717488786549025, 
		0.42190298621367914765, 3.0463625987357355872, 
		18.895299447327733204, 97.915189029455461554, 
		407.13940115493494659, 1274.3088990480582632, 
		2670.9883037012547506, 2815.7166284662544712, 
		1.2789926338424623394e-6, 8.6718263067604918916e-6, 
		1.3041508821299929489e-4, 0.001428224737372747892, 
		0.014684070635768789378, 0.13561403190404185755, 
		1.1152592585977393953, 8.0387088559465389038, 
		49.761318895895479206, 257.2684232313529138, 
		1066.8543146269566231, 3328.3874581009636362, 
		6948.8586598121634874, 7288.4893398212481055, 
		3.409350368197032893e-6, 2.3079025203103376076e-5, 
		3.4691373283901830239e-4, 0.003794994977222908545, 
		0.038974209677945602145, 0.3594948380414878371, 
		2.9522878893539528226, 21.246564609514287056, 
		131.28727387146173141, 677.38107093296675421, 
		2802.3724744545046518, 8718.5731420798254081, 
		18141.348781638832286, 18948.925349296308859
	};
	static double c[45] = {
		2.5568678676452702768e-15, 3.0393953792305924324e-14, 
		6.3343751991094840009e-13, 1.5041298011833009649e-11, 
		4.4569436918556541414e-10, 1.746393051427167951e-8, 
		1.0059224011079852317e-6, 1.0729838945088577089e-4, 
		0.05150322693642527738, 
		5.2527963991711562216e-15, 7.202118481421005641e-15, 
		7.2561421229904797156e-13, 1.482312146673104251e-11, 
		4.4602670450376245434e-10, 1.7463600061788679671e-8, 
		1.005922609132234756e-6, 1.0729838937545111487e-4, 
		0.051503226936437300716, 
		1.3365917359358069908e-14, -1.2932643065888544835e-13, 
		1.7450199447905602915e-12, 1.0419051209056979788e-11, 
		4.58047881980598326e-10, 1.7442405450073548966e-8, 
		1.0059461453281292278e-6, 1.0729837434500161228e-4, 
		0.051503226940658446941, 
		5.3771611477352308649e-14, -1.1396193006413731702e-12, 
		1.2858641335221653409e-11, -5.9802086004570057703e-11, 
		7.3666894305929510222e-10, 1.6731837150730356448e-8, 
		1.0070831435812128922e-6, 1.0729733111203704813e-4, 
		0.051503227360726294675, 
		3.7819492084858931093e-14, -4.8600496888588034879e-13, 
		1.6898350504817224909e-12, 4.5884624327524255865e-11, 
		1.2521615963377513729e-10, 1.8959658437754727957e-8, 
		1.0020716710561353622e-6, 1.073037119856927559e-4, 
		0.05150322383300230775
	};

	w = x > 0 ? x : -x;

	if (w < 8.5) {
		t = w * w * 0.0625;
		k = 13 * ((int) t);
		y = (((((((((((a[k] * t + a[k + 1]) * t + 
			a[k + 2]) * t + a[k + 3]) * t + a[k + 4]) * t + 
			a[k + 5]) * t + a[k + 6]) * t + a[k + 7]) * t + 
			a[k + 8]) * t + a[k + 9]) * t + a[k + 10]) * t + 
			a[k + 11]) * t + a[k + 12];
	} else if (w < 12.5) {
		k = (int) w;
		t = w - k;
		k = 14 * (k - 8);
		y = ((((((((((((b[k] * t + b[k + 1]) * t + 
			b[k + 2]) * t + b[k + 3]) * t + b[k + 4]) * t + 
			b[k + 5]) * t + b[k + 6]) * t + b[k + 7]) * t + 
			b[k + 8]) * t + b[k + 9]) * t + b[k + 10]) * t + 
			b[k + 11]) * t + b[k + 12]) * t + b[k + 13];
	} else {
		t = 60 / w;
		k = 9 * ((int) t);
		y = ((((((((c[k] * t + c[k + 1]) * t + 
			c[k + 2]) * t + c[k + 3]) * t + c[k + 4]) * t + 
			c[k + 5]) * t + c[k + 6]) * t + c[k + 7]) * t + 
			c[k + 8]) * sqrt(t) * exp(w);
	}
	return y;
}

static double alpha(double a)
{
	if (a <= 21) return 0;
	if (a <= 50) return 0.5842*pow(a-21,0.4)+0.07886*(a-21);
	return 0.1102*(a-8.7);
}


static double win(double n,int len,double alp,double iza)
{
	return dbesi0(alp*sqrt(1-4*n*n/(((double)len-1)*((double)len-1))))/iza;
}

static double sinc(double x)
{
	return x == 0 ? 1 : sin(x)/x;
}

static double hn_lpf(int n,double lpf,double fs)
{
	double t = 1/fs;
	double omega = 2*M_PI*lpf;
	return 2*lpf*t*sinc(n*omega*t);
}

static int gcd(int x, int y)
{
	int t;
	while (y != 0) {
		t = x % y;  x = y;  y = t;
	}
	return x;
}

static int CanResample(int sfrq,int dfrq)
{
	if (sfrq==dfrq) return 1;
	int frqgcd = gcd(sfrq,dfrq);

	if (dfrq>sfrq)
	{
		int fs1 = sfrq / frqgcd * dfrq;

		if (fs1/dfrq == 1) return 1;
		else if (fs1/dfrq % 2 == 0) return 1;
		else if (fs1/dfrq % 3 == 0) return 1;
		else return 0;
	}
	else
	{
		if (dfrq/frqgcd == 1) return 1;
		else if (dfrq/frqgcd % 2 == 0) return 1;
		else if (dfrq/frqgcd % 3 == 0) return 1;
		else return 0;
	}
}


CAudioFilter::CAudioFilter(void): m_peak(0.0), m_nch(0), m_sfrq(0), m_dfrq(0),
m_gain(1.0), m_bDownSample(true), AA(120), DF(100), FFTFIRLEN(16384)
{
}

bool CAudioFilter::SetFilterParam(const CAudioFilter::AfConfig& afConfig)
{
	if(!CanResample(afConfig.m_sfrq, afConfig.m_dfrq)) return false;

	if (afConfig.m_fast) {
		AA = 96;
		DF = 8000;
		FFTFIRLEN = 1024;
	} else {
		AA=120;
		DF=100;
		FFTFIRLEN=16384;
	}

	m_nch= afConfig.m_nch;
	m_sfrq = afConfig.m_sfrq;
	m_dfrq = afConfig.m_dfrq;

	double noiseamp = 0.18;
	m_gain = 1; 

	if(m_sfrq > m_dfrq) {
		m_bDownSample = true;
	} else if (m_sfrq < m_dfrq) {
		m_bDownSample = false;
	} else {
		return false;
	}
	return true;
}

CAudioFilter::~CAudioFilter(void)
{
}

bool CAudioFilter::DoSample(uint8_t* pInBuf, uint32_t inSize, bool ifEnd)
{
	if(m_bDownSample) {
		return downSample(pInBuf, inSize, ifEnd);
	}
	return upSample(pInBuf, inSize, ifEnd);
}

bool CAudioFilter::upSample(uint8_t* pInBuf, uint32_t inSize, bool ifEnd)
{
	__int64 fs1;
	int frqgcd,osf,fs2;
	REAL **stage1,*stage2;
	int n1,n1x,n1y,n2,n2b;
	int filter2len;
	int *f1order,*f1inc;
	int *fft_ip = NULL;
	REAL *fft_w = NULL;
	//unsigned char *rawinbuf,*rawoutbuf;
	REAL *inbuf,*outbuf;
	REAL **buf1,**buf2;
	int spcount = 0;
	int i,j;

	int n2b2;//=n2b/2;
	int rp;        // inbuf??fs1????????T???v?????????
	int ds;        // ????dispose????sfrq???T???v????
	int nsmplwrt1; // ?????t?@?C??????inbuf?????????l????v?Z????
	// stage2 filter??n?????T???v????
	int nsmplwrt2; // ?????t?@?C??????inbuf?????????l????v?Z????
	// stage2 filter??n?????T???v????
	int s1p;       // stage1 filter????o??????T???v???????n1y*osf????????]??
	int init;
	unsigned int sumread,sumwrite;
	int osc;
	REAL *ip,*ip_backup;
	int s1p_backup,osc_backup;
	int p;
	int inbuflen;
	int delay;// = 0;
	int delay2;

	m_peak = 0.0;
	filter2len = FFTFIRLEN; /* stage 2 filter length */

	/* Make stage 1 filter */
	{
		double aa = AA; /* stop band attenuation(dB) */
		double lpf,delta,d,df,alp,iza;
		double guard = 2;

		frqgcd = gcd(m_sfrq,m_dfrq);

		fs1 = (__int64)(m_sfrq / frqgcd) * (__int64)m_dfrq;

		if (fs1/m_dfrq == 1) osf = 1;
		else if (fs1/m_dfrq % 2 == 0) osf = 2;
		else if (fs1/m_dfrq % 3 == 0) osf = 3;
		else {
			//		  fprintf(stderr,"Resampling from %dHz to %dHz is not supported.\n",sfrq,dfrq);
			//		  fprintf(stderr,"%d/gcd(%d,%d)=%d must be divided by 2 or 3.\n",sfrq,sfrq,dfrq,fs1/dfrq);
			//		  exit(-1);
			return;
		}

		df = (m_dfrq*osf/2 - m_sfrq/2) * 2 / guard;
		lpf = m_sfrq/2 + (m_dfrq*osf/2 - m_sfrq/2)/guard;

		delta = pow(10.0,-aa/20);
		if (aa <= 21) d = 0.9222; else d = (aa-7.95)/14.36;

		n1 = fs1/df*d+1;
		if (n1 % 2 == 0) n1++;

		alp = alpha(aa);
		iza = dbesi0(alp);
		//printf("iza = %g\n",iza);

		n1y = fs1/m_sfrq;
		n1x = n1/n1y+1;

		f1order = (int*)malloc(sizeof(int)*n1y*osf);
		for(i=0;i<n1y*osf;i++) {
			f1order[i] = fs1/m_sfrq-(i*(fs1/(m_dfrq*osf)))%(fs1/m_sfrq);
			if (f1order[i] == fs1/m_sfrq) f1order[i] = 0;
		}

		f1inc = (int*)malloc(sizeof(int)*n1y*osf);
		for(i=0;i<n1y*osf;i++) {
			f1inc[i] = f1order[i] < fs1/(m_dfrq*osf) ? m_nch : 0;
			if (f1order[i] == fs1/m_sfrq) f1order[i] = 0;
		}

		stage1 = (REAL**)malloc(sizeof(REAL *)*n1y);
		stage1[0] = (REAL*)malloc(sizeof(REAL)*n1x*n1y);

		for(i=1;i<n1y;i++) {
			stage1[i] = &(stage1[0][n1x*i]);
			for(j=0;j<n1x;j++) stage1[i][j] = 0;
		}

		for(i=-(n1/2);i<=n1/2;i++)
		{
			stage1[(i+n1/2)%n1y][(i+n1/2)/n1y] = win(i,n1,alp,iza)*hn_lpf(i,lpf,fs1)*fs1/m_sfrq;
		}
	}

	/* Make stage 2 filter */

	{
		double aa = AA; /* stop band attenuation(dB) */
		double lpf,delta,d,df,alp,iza;
		int ipsize,wsize;

		delta = pow(10.0,-aa/20);
		if (aa <= 21) d = 0.9222; else d = (aa-7.95)/14.36;

		fs2 = m_dfrq*osf;

		for(i=1;;i = i * 2)
		{
			n2 = filter2len * i;
			if (n2 % 2 == 0) n2--;
			df = (fs2*d)/(n2-1);
			lpf = m_sfrq/2;
			if (df < DF) break;
		}

		alp = alpha(aa);

		iza = dbesi0(alp);

		for(n2b=1;n2b<n2;n2b*=2);
		n2b *= 2;

		stage2 = (REAL*)malloc(sizeof(REAL)*n2b);

		for(i=0;i<n2b;i++) stage2[i] = 0;

		for(i=-(n2/2);i<=n2/2;i++) {
			stage2[i+n2/2] = win(i,n2,alp,iza)*hn_lpf(i,lpf,fs2)/n2b*2;
		}

		ipsize    = 2+sqrt((double)n2b);
		fft_ip    = (int*)malloc(sizeof(int)*ipsize);
		fft_ip[0] = 0;
		wsize     = n2b/2;
		fft_w     = (REAL*)malloc(sizeof(REAL)*wsize);

		rdft(n2b,1,stage2,fft_ip,fft_w);
	}

	//	  delay=0;
	n2b2=n2b/2;

	buf1 = (REAL**)malloc(sizeof(REAL *)*m_nch);
	for(i=0;i<m_nch;i++)
	{
		buf1[i] = (REAL*)malloc(sizeof(REAL)*(n2b2/osf+1));
		for(j=0;j<(n2b2/osf+1);j++) buf1[i][j] = 0;
	}

	buf2 = (REAL**)malloc(sizeof(REAL *)*m_nch);
	for(i=0;i<m_nch;i++) buf2[i] = (REAL*)malloc(sizeof(REAL)*n2b);


	inbuf  = (REAL*)malloc(m_nch*(n2b2+n1x)*sizeof(REAL));
	outbuf = (REAL*)malloc(sizeof(REAL)*m_nch*(n2b2/osf+1));

	s1p = 0;
	rp  = 0;
	ds  = 0;
	osc = 0;

	init = 1;
	inbuflen = n1/2/(fs1/m_sfrq)+1;
	delay = (double)n2/2/(fs2/m_dfrq);
	delay2 = delay * m_nch;

	sumread = sumwrite = 0;

	{	/* Apply filters */
		
		int nsmplread,toberead,toberead2;
		unsigned int rv=0;
		int ch;

		toberead2 = toberead = floor((double)n2b2*m_sfrq/(m_dfrq*osf))+1+n1x-inbuflen;

		if (!ifEnd)
		{
			rv=m_nch*toberead;
			if (in_size<rv) return 0;
			nsmplread=toberead;
		}
		else
		{
			nsmplread=in_size/m_nch;
			rv=nsmplread*m_nch;
		}

		make_inbuf(nsmplread,inbuflen,rawinbuf,inbuf,toberead);

		inbuflen += toberead2;

		sumread += nsmplread;

		//nsmplwrt1 = ((rp-1)*sfrq/fs1+inbuflen-n1x)*m_dfrq*osf/sfrq;
		//if (nsmplwrt1 > n2b2) nsmplwrt1 = n2b2;
		nsmplwrt1 = n2b2;


		// apply stage 1 filter

		ip = &inbuf[((m_sfrq*(rp-1)+fs1)/fs1)*m_nch];

		s1p_backup = s1p;
		ip_backup  = ip;
		osc_backup = osc;

		for(ch=0;ch<m_nch;ch++)
		{
			REAL *op = &outbuf[ch];
			int fdo = fs1/(m_dfrq*osf),no = n1y*osf;

			s1p = s1p_backup; ip = ip_backup+ch;

			switch(n1x)
			{
			case 7:
				for(p=0;p<nsmplwrt1;p++)
		  {
			  int s1o = f1order[s1p];

			  buf2[ch][p] =
				  stage1[s1o][0] * *(ip+0*m_nch)+
				  stage1[s1o][1] * *(ip+1*m_nch)+
				  stage1[s1o][2] * *(ip+2*m_nch)+
				  stage1[s1o][3] * *(ip+3*m_nch)+
				  stage1[s1o][4] * *(ip+4*m_nch)+
				  stage1[s1o][5] * *(ip+5*m_nch)+
				  stage1[s1o][6] * *(ip+6*m_nch);

			  ip += f1inc[s1p];

			  s1p++;
			  if (s1p == no) s1p = 0;
		  }
				break;

			case 9:
				for(p=0;p<nsmplwrt1;p++)
		  {
			  int s1o = f1order[s1p];

			  buf2[ch][p] =
				  stage1[s1o][0] * *(ip+0*m_nch)+
				  stage1[s1o][1] * *(ip+1*m_nch)+
				  stage1[s1o][2] * *(ip+2*m_nch)+
				  stage1[s1o][3] * *(ip+3*m_nch)+
				  stage1[s1o][4] * *(ip+4*m_nch)+
				  stage1[s1o][5] * *(ip+5*m_nch)+
				  stage1[s1o][6] * *(ip+6*m_nch)+
				  stage1[s1o][7] * *(ip+7*m_nch)+
				  stage1[s1o][8] * *(ip+8*m_nch);

			  ip += f1inc[s1p];

			  s1p++;
			  if (s1p == no) s1p = 0;
		  }
				break;

			default:
				for(p=0;p<nsmplwrt1;p++)
		  {
			  REAL tmp = 0;
			  REAL *ip2=ip;

			  int s1o = f1order[s1p];

			  for(i=0;i<n1x;i++)
			  {
				  tmp += stage1[s1o][i] * *ip2;
				  ip2 += m_nch;
			  }
			  buf2[ch][p] = tmp;

			  ip += f1inc[s1p];

			  s1p++;
			  if (s1p == no) s1p = 0;
		  }
				break;
			}

			osc = osc_backup;

			// apply stage 2 filter

			for(p=nsmplwrt1;p<n2b;p++) buf2[ch][p] = 0;

			//for(i=0;i<n2b2;i++) printf("%d:%g ",i,buf2[ch][i]);

			rdft(n2b,1,buf2[ch],fft_ip,fft_w);

			buf2[ch][0] = stage2[0]*buf2[ch][0];
			buf2[ch][1] = stage2[1]*buf2[ch][1]; 

			for(i=1;i<n2b/2;i++)
			{
				REAL re,im;

				re = stage2[i*2  ]*buf2[ch][i*2] - stage2[i*2+1]*buf2[ch][i*2+1];
				im = stage2[i*2+1]*buf2[ch][i*2] + stage2[i*2  ]*buf2[ch][i*2+1];

				//printf("%d : %g %g %g %g %g %g\n",i,stage2[i*2],stage2[i*2+1],buf2[ch][i*2],buf2[ch][i*2+1],re,im);

				buf2[ch][i*2  ] = re;
				buf2[ch][i*2+1] = im;
			}

			rdft(n2b,-1,buf2[ch],fft_ip,fft_w);

			for(i=osc,j=0;i<n2b2;i+=osf,j++)
			{
				REAL f = (buf1[ch][j] + buf2[ch][i]);
				op[j*m_nch] = f;
			}

			nsmplwrt2 = j;

			osc = i - n2b2;

			for(j=0;i<n2b;i+=osf,j++)
				buf1[ch][j] = buf2[ch][i];

		}

		rp += nsmplwrt1 * (m_sfrq / frqgcd) / osf;


		make_outbuf(nsmplwrt2,outbuf,delay2);

		if (!init) {
			if (ifEnd) {
				if ((double)sumread*m_dfrq/m_sfrq+2 > sumwrite+nsmplwrt2) {

					sumwrite += nsmplwrt2;
				} else {

				}
			} else {
				sumwrite += nsmplwrt2;
			}
		} else {
			if (nsmplwrt2 < delay) {
				delay -= nsmplwrt2;
			} else {
				if (ifEnd) {
					if ((double)sumread*m_dfrq/m_sfrq+2 > sumwrite+nsmplwrt2-delay) {
						sumwrite += nsmplwrt2-delay;
					} else {

					}
				} else {

					sumwrite += nsmplwrt2-delay;
					init = 0;
				}
			}
		}

		{
			int ds = (rp-1)/(fs1/m_sfrq);

			assert(inbuflen >= ds);

			mem_ops<REAL>::move(inbuf,inbuf+m_nch*ds,m_nch*(inbuflen-ds));
			inbuflen -= ds;
			rp -= ds*(fs1/m_sfrq);
		}

		return rv;
	}
	
	free(f1order);
	free(f1inc);
	free(stage1[0]);
	free(stage1);
	free(stage2);
	free(fft_ip);
	free(fft_w);
	for(i=0;i<m_nch;i++) free(buf1[i]);
	free(buf1);
	for(i=0;i<m_nch;i++) free(buf2[i]);
	free(buf2);
	free(inbuf);
	free(outbuf);


}

bool CAudioFilter::downSample(uint8_t* pInBuf, uint32_t inSize)
{

}

class CUpSampler : public CAudioFilter
{
	
};