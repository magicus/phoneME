/*
 * @(#)QtDebugBackEnd.cpp	1.9 06/10/25
 * 
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

#include <unistd.h>

#include "QtDebugBackEnd.h"
#include "java_awt_QtDebugBackEnd.h"

static jobject monsterLock;

QtDebugBackEnd::QtDebugBackEnd(JNIEnv *env) : QServerSocket( 8083, 5)
{	
	env->GetJavaVM(&myJVM);

	QtDebugJava = (jclass)(env->NewGlobalRef(env->FindClass("java/awt/QtDebugBackEnd")));
	serviceSocketMID = env->GetStaticMethodID(QtDebugJava, "serviceSocket", "(I)V");
	
	jclass cls = env->FindClass("java/awt/QtGraphics");
	jfieldID fld = env->GetStaticFieldID(cls, "MONSTERLOCK", "Ljava/lang/Object;");
	monsterLock = env->NewGlobalRef(env->GetStaticObjectField(cls, fld));
	
}

void QtDebugBackEnd::newConnection(int socket)
{

	JNIEnv *env;
	
	myJVM->GetEnv((void **)&env, JNI_VERSION_1_2);
/*	
	env->CallStaticVoidMethod(QtDebugJava, serviceSocketMID, (jint)socket);
*/
	
	Java_java_awt_QtDebugBackEnd_processSocket(env, QtDebugJava, socket);	
}

static void sendIndex(int socket)
{
	int i;
	char response[512];
	
	QSocketDevice qsd(socket, QSocketDevice::Stream);
	
	strcpy(response, "HTTP/1.0 200 OK\nContent-Type: text/html\n\r\n");	
	qsd.writeBlock(response, strlen(response));
	
	strcpy(response, "<HTML>\n<HEAD>Basis AWT BackEnd</HEAD>\n<BODY>\n");
	qsd.writeBlock(response, strlen(response));
	
	strcpy(response, "<TABLE border=1>\n<CAPTION>Graphics Descriptors Allocated</CAPTION>\n");
	strcat(response, "<TR><TH>Desc</TH><TH>Clip</TH><TH>Clipped</TH><TH>extraAlpha</TH><TH>rasterOp</TH><TH>currentAlpha</TH><TH>blendMode</TH></TR>\n");
	qsd.writeBlock(response, strlen(response));	
	
	
	for(i=NumGraphDescriptors-1; i>=0; i--)
	{
		if(QtGraphDescPool[i].used != 0)
		{
			sprintf(response,
					"<TR><TH>G%d</TH><TD>%d,%d %d,%d</TD><TD>%s</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD></TR>\n",
				   i,
				   QtGraphDescPool[i].clip[0], QtGraphDescPool[i].clip[1],
				   QtGraphDescPool[i].clip[2], QtGraphDescPool[i].clip[3],
				   QtGraphDescPool[i].clipped?"True":"False",
				   QtGraphDescPool[i].extraalpha,
				   QtGraphDescPool[i].rasterOp,
				   QtGraphDescPool[i].currentalpha,
				   QtGraphDescPool[i].blendmode );
			
			qsd.writeBlock(response, strlen(response));	
		}
	}
	
	strcpy(response, "</TABLE><P><P><P><P><P>\n");
	qsd.writeBlock(response, strlen(response));	
	
	strcpy(response, "<TABLE border=1>\n<CAPTION>Image Descriptors Allocated</CAPTION>\n");
	strcat(response, "<TR><TH>Desc</TH><TH>refCount</TH><TH>width</TH><TH>height</TH><TH>loadBuffer</TH></TR>\n");
	qsd.writeBlock(response, strlen(response));		
	
	for(i=NumImageDescriptors-1; i>=0; i--)
	{
		if(QtImageDescPool[i].count > 0)
		{
			sprintf(response,
					"<TR><TH>I%d</TH><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%s</TD></TR>\n",
				   i,
				   QtImageDescPool[i].count,
				   QtImageDescPool[i].width,
				   QtImageDescPool[i].height,
				   QtImageDescPool[i].loadBuffer != NULL ? "True": "False");

			qsd.writeBlock(response, strlen(response));	
		}
	}	

	strcpy(response, "</TABLE><P><P><P><P><P>\n");
	qsd.writeBlock(response, strlen(response));	
		

	QHostAddress qha = qsd.address();
	QString qhas = qha.toString();
	const char *addstr = qhas.latin1();
	int port = (int)qsd.port();	
	
	for(i=NumImageDescriptors-1; i>=0; i--)
	{
		if(QtImageDescPool[i].count > 0)
		{
			sprintf(response,
					"<P>I%d:\n<IMG src=\"http://%s:%d/pic%d.png\">\n",
					i,
					addstr,
					port,
					i);
			
			qsd.writeBlock(response, strlen(response));	
		}
	}
	
	strcpy(response, "</BODY>\n</HTML>\n");
	
	qsd.writeBlock(response, strlen(response));	
}

static void sendImage(int socket, int imgDesc)
{
	if(QtImageDescPool[imgDesc].count <= 0) return;
	
	char response[512];
	
	QSocketDevice qsd(socket, QSocketDevice::Stream);
	QDataStream qds(&qsd);
	
	strcpy(response, "HTTP/1.0 200 OK\nContent-Type: image/png\n\r\n");
	qsd.writeBlock(response, strlen(response));
	
	/* FOR VIEWABLE SCREEN 0 !! */
	if(imgDesc!=0) {
		if(QtImageDescPool[imgDesc].loadBuffer != NULL)
			qds << *(QtImageDescPool[imgDesc].loadBuffer);
		else
			qds << *((const QPixmap *)(QtImageDescPool[imgDesc].qpd));
	}
	else
	{
		QPixmap p(QtImageDescPool[0].width, QtImageDescPool[0].height);
		bitBlt(&p, 0, 0, QtImageDescPool[0].qpd, 0, 0, QtImageDescPool[0].width, QtImageDescPool[0].height, Qt::CopyROP, false);
		qds << p;
	}
}

JNIEXPORT void JNICALL 
Java_java_awt_QtDebugBackEnd_processSocket(JNIEnv *env, jclass cls, jint socket)
{
	int r;
	char *b, *c;
	char buffer[512];
	char tmpstr[512];
	
	memset(buffer, '\0', sizeof(buffer));
	while ((r = read(socket, buffer, sizeof(buffer)-1)) > 0)
	{
		b = strstr(buffer, "GET");
		if(b == NULL) return;
		
		memset(tmpstr, '\0', sizeof(tmpstr));
		if(sscanf(b, "%*s %s %*s", tmpstr) != 1) return;
		
		if(strcasestr(tmpstr, "index") != NULL)
		{
			if(env->MonitorEnter(monsterLock) == JNI_OK)
			{
				sendIndex(socket);
				env->MonitorExit(monsterLock);
			}
		}
		else if ((c=strcasestr(tmpstr, "pic")) != NULL)
		{
			int n;
			if(sscanf(c+3, "%d", &n) != 1) return;
			if(env->MonitorEnter(monsterLock) == JNI_OK)
			{
				sendImage(socket, n);
				env->MonitorExit(monsterLock);
			}	
		}
		memset(buffer, '\0', sizeof(buffer));
	}
	
	close(socket);
}



/* This debug info is only meaningful for Basis. */

void debug_dump_desc()
{
	int i;
	
	for(i=NumGraphDescriptors-1; i>=0; i--)
	{
		if(QtGraphDescPool[i].used != 0)
		{
			printf("GDESC:%d cl:(%d,%d %d,%d) cl?%d ea:%d ro:%d ca:%d bm:%d\n",
				   i,
				   QtGraphDescPool[i].clip[0], QtGraphDescPool[i].clip[1],
				   QtGraphDescPool[i].clip[2], QtGraphDescPool[i].clip[3],
				   QtGraphDescPool[i].clipped,
				   QtGraphDescPool[i].extraalpha,
				   QtGraphDescPool[i].rasterOp,
				   QtGraphDescPool[i].currentalpha,
				   QtGraphDescPool[i].blendmode );
		}
	}
	
	for(i=NumImageDescriptors-1; i>=0; i--)
	{
		if(QtImageDescPool[i].count > 0)
		{
			printf("IDESC:%d w:%d h:%d lb?%d",
				   QtImageDescPool[i].count,
				   QtImageDescPool[i].width,
				   QtImageDescPool[i].height,
				   QtImageDescPool[i].loadBuffer != NULL ? 1 : 0);
		}
	}
	
}

void debug_dump_images()
{
	int i;
	char filename[128];
	char timestring[60];
	time_t now;
	
	now = time(NULL);
	
	strftime(timestring, sizeof(timestring), "%Y" "%m" "%d" "%H" "%M", localtime(&now));
	
	for(i=NumImageDescriptors-1; i>0; i--)
	{
		if(QtImageDescPool[i].count > 0)
		{
			sprintf(filename, "/tmp/%s-%d.bmp", timestring, i);
			
			if(QtImageDescPool[i].loadBuffer != NULL)
				QtImageDescPool[i].loadBuffer->save(QString(filename), "BMP");
			else
				((QPixmap *)QtImageDescPool[i].qpd)->save(QString(filename), "BMP");
		}
	}
}


void debug_dump_image(int imgpsd)
{
	char filename[128];
	char timestring[60];
	time_t now;
	
	now = time(NULL);
	
	strftime(timestring, sizeof(timestring), "%Y" "%m" "%d" "%H" "%M", localtime(&now));
	
	if(QtImageDescPool[imgpsd].count > 0)
	{
		sprintf(filename, "/tmp/%s-%d.bmp", timestring, imgpsd);
		
		if(QtImageDescPool[imgpsd].loadBuffer != NULL)
			QtImageDescPool[imgpsd].loadBuffer->save(QString(filename), "BMP");
		else
			((QPixmap *)QtImageDescPool[imgpsd].qpd)->save(QString(filename), "BMP");
	}
}


