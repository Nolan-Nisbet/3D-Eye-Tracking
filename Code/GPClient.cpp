//////////////////////////////////////////////////////////////////////////////////////
// GPClient.cpp - Example class using Open Gaze API
// Written in 2013 by Gazepoint www.gazept.com
//
// To the extent possible under law, the author(s) have dedicated all copyright 
// and related and neighboring rights to this software to the public domain worldwide. 
// This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication along with this 
// software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//////////////////////////////////////////////////////////////////////////////////////


#define  _WINSOCK_DEPRECATED_NO_WARNINGS

#include "stdafx.h"
#include "GPClient.h"


#define RX_TCP_BUFFER_MAX 64000

GPClient::GPClient(void)
{
	SYSTEMTIME systime;
	FILETIME ftime;
	LARGE_INTEGER stime;

	_rx_mutex = CreateMutex(NULL, FALSE, "rx_mutex");
	_tx_mutex = CreateMutex(NULL, FALSE, "tx_mutex");
	_timer = CreateWaitableTimer(NULL, FALSE, "GPClientTimer");

	// 10 ms update rate
	GetLocalTime(&systime);
	SystemTimeToFileTime(&systime, &ftime);
	LocalFileTimeToFileTime(&ftime, (FILETIME *)&stime);
	stime.QuadPart += (10 * 10000);
	SetWaitableTimer(_timer, &stime, 10, 0, 0, 0);

	_ip_port = 4242;
	_ip_address = "127.0.0.1";

	// By default keep 60 records/sec * 60 seconds * 3 minutes of data
	// If we kept ALL records but never read them we could run out of memory
	_rx_buffer_size = 60 * 60 * 3;
	_rx_status = FALSE;
	_connected_status = FALSE;
}

GPClient::~GPClient(void)
{
	client_disconnect();

	CancelWaitableTimer(_timer);

	CloseHandle(_timer);
	CloseHandle(_rx_mutex);
	CloseHandle(_tx_mutex);
}

void GPClient::client_connect()
{
	TSTRUCT *pts = new TSTRUCT;
	pts->_tptr = this;

	WaitForSingleObject(_rx_mutex, INFINITE);
	_tx_buffer.clear();
	_rx_buffer.clear();
	ReleaseMutex(_rx_mutex);

	AfxBeginThread(GPClientThread, pts);
	Sleep(200);
}

void GPClient::client_disconnect()
{
	WaitForSingleObject(_rx_mutex, INFINITE);
	_thread_exit = TRUE;
	ReleaseMutex(_rx_mutex);

	Sleep(100);

	WaitForSingleObject(_rx_mutex, INFINITE);
	_rx_buffer.clear();
	_tx_buffer.clear();
	ReleaseMutex(_rx_mutex);
}

UINT GPClient::GPClientThread(LPVOID param)
{
	unsigned int result;
	unsigned int delimiter_index;
	string rxstr;
	char rxbuffer[RX_TCP_BUFFER_MAX];
	int state = 0;
	double rx_time = GetTickCount();

	TSTRUCT*	ts = (TSTRUCT*)param;

	SOCKADDR_IN addr;

	WSADATA wsadata;
	SOCKET ipsocket;
	u_long poll = TRUE;

	if (WSAStartup(0x0101, &wsadata))
	{
		return 0;
	}

	ipsocket = socket(AF_INET, SOCK_STREAM, 0);
	if (ipsocket == INVALID_SOCKET)
	{
		return 0;
	}

	if (ioctlsocket(ipsocket, FIONBIO, &poll) == SOCKET_ERROR)
	{
		return 0;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(ts->_tptr->_ip_port);
	addr.sin_addr.S_un.S_addr = inet_addr(ts->_tptr->_ip_address.c_str());

	connect(ipsocket, (struct sockaddr*)&addr, sizeof(addr));

	ts->_tptr->_thread_exit = FALSE;

	Sleep(250);

	ts->_tptr->_rx_status = FALSE;
	ts->_tptr->_connected_status = TRUE;

	do
	{
		WaitForSingleObject(ts->_tptr->_timer, 250);

		// if data is not received within last 4 second then rx is not active
		if (GetTickCount() > rx_time + 4000)
		{
			ts->_tptr->_rx_status = FALSE;
		}

		if (ipsocket != NULL && ts->_tptr->_thread_exit != TRUE)
		{
			do
			{
				result = recv(ipsocket, rxbuffer, RX_TCP_BUFFER_MAX, 0);

				if (result == SOCKET_ERROR)
				{
					state = WSAGetLastError();
				}
				else if (result > 0)
				{
					rx_time = GetTickCount();
					ts->_tptr->_rx_status = TRUE;

					// save room to add a NULL character to incoming char stream
					if (result > RX_TCP_BUFFER_MAX - 1)
					{
						result = RX_TCP_BUFFER_MAX - 1;
					}

					rxbuffer[result] = NULL;
					rxstr = rxstr + rxbuffer;

					// find end of record delimiter
					delimiter_index = rxstr.find("\r\n", 0);

					WaitForSingleObject(ts->_tptr->_rx_mutex, INFINITE);
					while (delimiter_index != string::npos)
					{
						string tmp = rxstr.substr(0, delimiter_index);

						// save record at head of queue (FIFO)
						ts->_tptr->_rx_buffer.push_back(tmp);

						// remove records longer than queue size (so we don't run out of memory)
						while (ts->_tptr->_rx_buffer.size() > ts->_tptr->_rx_buffer_size)
						{
							ts->_tptr->_rx_buffer.pop_front();
						}

						rxstr.erase(0, delimiter_index + 2);
						delimiter_index = rxstr.find("\r\n", 0);
					}
					ReleaseMutex(ts->_tptr->_rx_mutex);

				}
			} while (result > 0 && result != SOCKET_ERROR && ts->_tptr->_thread_exit != TRUE);

			WaitForSingleObject(ts->_tptr->_tx_mutex, INFINITE);
			for (unsigned int i = 0; i < ts->_tptr->_tx_buffer.size(); i++)
			{
				send(ipsocket, ts->_tptr->_tx_buffer.at(i).c_str(), ts->_tptr->_tx_buffer.at(i).size(), 0);
			}

			ts->_tptr->_tx_buffer.clear();
			ReleaseMutex(ts->_tptr->_tx_mutex);
		}
	} while (ts->_tptr->_thread_exit != TRUE);

	closesocket(ipsocket);

	WSACleanup();

	ts->_tptr->_connected_status = FALSE;

	delete param;
	param = NULL;

	return TRUE;
}

void GPClient::send_cmd(string cmd)
{
	cmd = cmd + "\r\n";

	WaitForSingleObject(_tx_mutex, INFINITE);
	_tx_buffer.push_back(cmd);
	ReleaseMutex(_tx_mutex);
}

string GPClient::get_rx_latest()
{
	string tmp;

	WaitForSingleObject(_rx_mutex, INFINITE);
	if (_rx_buffer.size() > 0)
	{
		tmp = _rx_buffer.back();
		_rx_buffer.clear();
	}
	ReleaseMutex(_rx_mutex);

	return tmp;
}

void GPClient::get_rx(deque <string> &data)
{
	WaitForSingleObject(_rx_mutex, INFINITE);
	data.clear();
	_rx_buffer.swap(data);
	ReleaseMutex(_rx_mutex);
}

bool GPClient::get_rx_status()
{
	return _rx_status;
}

bool GPClient::is_connected()
{
	return _connected_status;
}
