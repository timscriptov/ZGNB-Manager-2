#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/errqueue.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#ifdef USE_IDN
#include <idna.h>
#include <locale.h>
#endif

#include <stdarg.h>
#include "com_netease_LDNetDiagnoService_LDNetTraceRoute.h"
#include "Provider.h"
#define OUT_LEN 1000

JavaVM *gJvm = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int isFirst;
char* jstringTostring(JNIEnv* env, jstring jstr);
JNIEXPORT void JNICALL Java_com_netease_LDNetDiagnoService_LDNetTraceRoute_startJNICTraceRoute(JNIEnv *env, jobject obj, jstring command){
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "===============begin=====================" );
	(*env)->GetJavaVM(env, &gJvm);
	(*gJvm)->AttachCurrentThread(gJvm, &env, NULL);

	isFirst = 1;
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "startTraceCJNI c_command begin...." );
	char* c_command = jstringTostring(env, command);
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "startTraceCJNI c_command end...." );
	char* argv[]={"tracepath", c_command};
	mainTracePath(2, argv);
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "===============end=====================" );
}

char* jstringTostring(JNIEnv* env, jstring jstr){
	char* rtn = NULL;
	jclass clsstring = (*env)->FindClass(env, "java/lang/String");
	jstring strencode = (*env)->NewStringUTF(env, "utf-8");
	jmethodID mid = (*env)->GetMethodID(env, clsstring, "getBytes", "(Ljava/lang/String;)[B");
	jbyteArray barr= (jbyteArray)(*env)->CallObjectMethod(env, jstr, mid, strencode);
	jsize alen = (*env)->GetArrayLength(env, barr);
	jbyte* ba = (*env)->GetByteArrayElements(env, barr, JNI_FALSE);
	if (alen > 0)
	{
		rtn = (char*)malloc(alen + 1);
		memcpy(rtn, ba, alen);
		rtn[alen] = 0;
	}
	(*env)->ReleaseByteArrayElements(env, barr, ba, 0);
	return rtn;
}

int printf0(const char *fmt, ...){
	va_list argptr;
	int cnt;
	va_start(argptr, fmt);
	char *buffer = (char *)malloc(OUT_LEN);
	memset(buffer, OUT_LEN, 0);
	cnt = vsnprintf(buffer, OUT_LEN, fmt, argptr);
	buffer[cnt] = '\0';
	pthread_mutex_lock(&mutex);
	PrintTraceInfo(buffer);
	pthread_mutex_unlock(&mutex);
	free(buffer);
	va_end(argptr);
	isFirst++;
	return 1;
}



#ifndef IP_PMTUDISC_PROBE
#define IP_PMTUDISC_PROBE	3
#endif

#define MAX_HOPS_LIMIT		255
#define MAX_HOPS_DEFAULT	30

struct hhistory
{
	int	hops;
	struct timeval sendtime;
};

struct hhistory his[64];
int hisptr;

struct sockaddr_in target;
__u16 base_port;
int max_hops = MAX_HOPS_DEFAULT;

const int overhead = 28;
int mtu = 65535;
void *pktbuf;
int hops_to = -1;
int hops_from = -1;
int no_resolve = 0;
int show_both = 0;

#define HOST_COLUMN_SIZE	24

struct probehdr
{
	__u32 ttl;
	struct timeval tv;
};

void data_wait(int fd)
{
	fd_set fds;
	struct timeval tv;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	select(fd+1, &fds, NULL, NULL, &tv);
}

void print_host(const char *a, const char *b, int both)
{
	int plen;
	plen = printf0("%s", a);
	if (both)
		plen += printf0(" (%s)", b);
	if (plen >= HOST_COLUMN_SIZE)
		plen = HOST_COLUMN_SIZE - 1;
	printf0("%*s", HOST_COLUMN_SIZE - plen, "");
}

int recverr(int fd, int ttl)
{
	int res;
	struct probehdr rcvbuf;
	char cbuf[512];
	struct iovec  iov;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct sock_extended_err *e;
	struct sockaddr_in addr;
	struct timeval tv;
	struct timeval *rettv;
	int slot;
	int rethops;
	int sndhops;
	int progress = -1;
	int broken_router;

restart:
	memset(&rcvbuf, -1, sizeof(rcvbuf));
	iov.iov_base = &rcvbuf;
	iov.iov_len = sizeof(rcvbuf);
	msg.msg_name = (__u8*)&addr;
	msg.msg_namelen = sizeof(addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;
	msg.msg_control = cbuf;
	msg.msg_controllen = sizeof(cbuf);

	gettimeofday(&tv, NULL);
	res = recvmsg(fd, &msg, MSG_ERRQUEUE);
	if (res < 0) {
		if (errno == EAGAIN)
			return progress;
		goto restart;
	}

	progress = mtu;

	rethops = -1;
	sndhops = -1;
	e = NULL;
	rettv = NULL;
	slot = ntohs(addr.sin_port) - base_port;
	if (slot>=0 && slot < 63 && his[slot].hops) {
		sndhops = his[slot].hops;
		rettv = &his[slot].sendtime;
		his[slot].hops = 0;
	}
	broken_router = 0;
	if (res == sizeof(rcvbuf)) {
		if (rcvbuf.ttl == 0 || rcvbuf.tv.tv_sec == 0) {
			broken_router = 1;
		} else {
			sndhops = rcvbuf.ttl;
			rettv = &rcvbuf.tv;
		}
	}

	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == SOL_IP) {
			if (cmsg->cmsg_type == IP_RECVERR) {
				e = (struct sock_extended_err *) CMSG_DATA(cmsg);
			} else if (cmsg->cmsg_type == IP_TTL) {
				memcpy(&rethops, CMSG_DATA(cmsg), sizeof(rethops));
			} else {
				printf0("cmsg:%d\n ", cmsg->cmsg_type);
			}
		}
	}
	if (e == NULL) {
		printf0("no info\n");
		return 0;
	}
	if (e->ee_origin == SO_EE_ORIGIN_LOCAL) {
		printf0("%2d?: %*s ", ttl, -(HOST_COLUMN_SIZE - 1), "[LOCALHOST]");
	} else if (e->ee_origin == SO_EE_ORIGIN_ICMP) {
		char abuf[128];
		struct sockaddr_in *sin = (struct sockaddr_in*)(e+1);
		struct hostent *h = NULL;
		char *idn = NULL;

		inet_ntop(AF_INET, &sin->sin_addr, abuf, sizeof(abuf));

		if (sndhops>0)
			printf0("%2d:  ", sndhops);
		else
			printf0("%2d?: ", ttl);

		if (!no_resolve || show_both) {
			fflush(stdout);
			h = gethostbyaddr((char *) &sin->sin_addr, sizeof(sin->sin_addr), AF_INET);
		}

#ifdef USE_IDN
		if (h && idna_to_unicode_lzlz(h->h_name, &idn, 0) != IDNA_SUCCESS)
			idn = NULL;
#endif
		if (no_resolve)
			print_host(abuf, h ? (idn ? idn : h->h_name) : abuf, show_both);
		else
			print_host(h ? (idn ? idn : h->h_name) : abuf, abuf, show_both);

#ifdef USE_IDN
		free(idn);
#endif
	}

	if (rettv) {
		int diff = (tv.tv_sec-rettv->tv_sec)*1000000+(tv.tv_usec-rettv->tv_usec);
		printf0("%3d.%03dms ", diff/1000, diff%1000);
		if (broken_router)
			printf0("(This broken router returned corrupted payload) ");
	}

	switch (e->ee_errno) {
	case ETIMEDOUT:
		printf0("\n");
		break;
	case EMSGSIZE:
		printf0("pmtu %d\n", e->ee_info);
		mtu = e->ee_info;
		progress = mtu;
		break;
	case ECONNREFUSED:
		printf0("reached\n");
		hops_to = sndhops<0 ? ttl : sndhops;
		hops_from = rethops;
		return 0;
	case EPROTO:
		printf0("!P\n");
		return 0;
	case EHOSTUNREACH:
		if (e->ee_origin == SO_EE_ORIGIN_ICMP &&
		    e->ee_type == 11 &&
		    e->ee_code == 0) {
			if (rethops>=0) {
				if (rethops<=64)
					rethops = 65-rethops;
				else if (rethops<=128)
					rethops = 129-rethops;
				else
					rethops = 256-rethops;
				
			}
			printf0("\n");
			break;
		}
		printf0("!H\n");
		return 0;
	case ENETUNREACH:
		printf0("!N\n");
		return 0;
	case EACCES:
		printf0("!A\n");
		return 0;
	default:
		printf0("\n");
		errno = e->ee_errno;
		perror("NET ERROR");
		return 0;
	}
	goto restart;
}

int probe_ttl(int fd, int ttl)
{
	int i;
	struct probehdr *hdr = pktbuf;

	memset(pktbuf, 0, mtu);
restart:
	for (i=0; i<2; i++) {
		int res;

		hdr->ttl = ttl;
		target.sin_port = htons(base_port + hisptr);
		gettimeofday(&hdr->tv, NULL);
		his[hisptr].hops = ttl;
		his[hisptr].sendtime = hdr->tv;
		if (sendto(fd, pktbuf, mtu-overhead, 0, (struct sockaddr*)&target, sizeof(target)) > 0)
			break;
		res = recverr(fd, ttl);
		his[hisptr].hops = 0;
		if (res==0)
			return 0;
		if (res > 0)
			goto restart;
	}
	hisptr = (hisptr + 1)&63;

	if (i<2) {
		data_wait(fd);
		if (recv(fd, pktbuf, mtu, MSG_DONTWAIT) > 0) {
			printf0("%2d?: reply received 8)\n", ttl);
			return 0;
		}
		return recverr(fd, ttl);
	}

	printf0("%2d:  send failed\n", ttl);
	return 0;
}

static int usage(void);

static int usage(void)
{
	printf0("Usage: tracepath [-n] [-b] [-l <len>] [-p port] <destination>\n");
	return -1;
}

int
mainTracePath(int argc, char **argv)
{
	struct hostent *he;
	int fd;
	int on;
	int ttl;
	char *p;
	int ch;
#ifdef USE_IDN
	int rc;
	setlocale(LC_ALL, "");
#endif

	while ((ch = getopt(argc, argv, "nbh?l:m:p:")) != EOF) {
		switch(ch) {
		case 'n':
			no_resolve = 1;
			break;
		case 'b':
			show_both = 1;
			break;
		case 'l':
			if ((mtu = atoi(optarg)) <= overhead) {
				printf0("Error: pktlen must be > %d and <= %d.\n",
					overhead, INT_MAX);
				return -1;
			}
			break;
		case 'm':
			max_hops = atoi(optarg);
			if (max_hops < 0 || max_hops > MAX_HOPS_LIMIT) {
				printf0(
					"Error: max hops must be 0 .. %d (inclusive).\n",
					MAX_HOPS_LIMIT);
			}
			break;
		case 'p':
			base_port = atoi(optarg);
			break;
		default:
			return usage();
			break;
		}
	}

	argc -= optind;
	argv += optind;


	if (argc != 1)
		return usage();

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		printf0("socket: cant create socket");
		return -1;
	}
	target.sin_family = AF_INET;

	if (!base_port) {
		p = strchr(argv[0], '/');
		if (p) {
			*p = 0;
			base_port = atoi(p+1);
		} else
			base_port = 44444;
	}

	p = argv[0];
#ifdef USE_IDN
	rc = idna_to_ascii_lz(argv[0], &p, 0);
	if (rc != IDNA_SUCCESS) {
		printf0("IDNA encoding failed: %s\n", idna_strerror(rc));
		return -2;
	}
#endif

	he = gethostbyname(p);
	if (he == NULL) {
		printf0("gethostbyname: cant get host from hostname");
		return -1;
	}

#ifdef USE_IDN
	free(p);
#endif

	memcpy(&target.sin_addr, he->h_addr, 4);

	on = IP_PMTUDISC_PROBE;
	if (setsockopt(fd, SOL_IP, IP_MTU_DISCOVER, &on, sizeof(on)) &&
	    (on = IP_PMTUDISC_DO,
	     setsockopt(fd, SOL_IP, IP_MTU_DISCOVER, &on, sizeof(on)))) {
		printf0("IP_MTU_DISCOVER error");
		return -1;
	}
	on = 1;
	if (setsockopt(fd, SOL_IP, IP_RECVERR, &on, sizeof(on))) {
		printf0("IP_RECVERR error");
		return -1;
	}
	if (setsockopt(fd, SOL_IP, IP_RECVTTL, &on, sizeof(on))) {
		printf0("IP_RECVTTL error");
		return -1;
	}

	pktbuf = malloc(mtu);
	if (!pktbuf) {
		printf0("malloc pktbuf error");
		return -1;
	}

	int timeoutTTL = 0;
	for (ttl = 1; ttl <= max_hops; ttl++) {
		int res;
		int i;

		on = ttl;
		if (setsockopt(fd, SOL_IP, IP_TTL, &on, sizeof(on))) {
			printf0("IP_TTL error");
			return -1;
		}

restart:
		for (i=0; i<1; i++) {
			int old_mtu;

			old_mtu = mtu;
			res = probe_ttl(fd, ttl);
			if (mtu != old_mtu)
				goto restart;
			if (res == 0)
				goto done;
			if (res > 0){
				timeoutTTL = 0;
				break;
			}
		}

		if (res < 0){
			if(timeoutTTL >= 3){
				return 0;
			}else {
				timeoutTTL++;
				printf0("%2d:  **********", ttl);
			}
		}
	}
	printf0("     Too many hops: pmtu %d\n", mtu);
done:
	printf0("     Resume: pmtu %d ", mtu);
	if (hops_to>=0)
		printf0("hops %d ", hops_to);
	if (hops_from>=0)
		printf0("back %d ", hops_from);
	printf0("\n");

	return 0;
}
