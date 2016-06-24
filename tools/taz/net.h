/*
 * Interface for connections to instruments
 * @author tweber
 * @date mar-2016
 * @license GPLv2
 */
 
#ifndef __TAKIN_NET_IF_H__
#define __TAKIN_NET_IF_H__

#include <string>
#include <QObject>
#include <QString>

#include "tasoptions.h"
#include "dialogs/NetCacheDlg.h"


class NetCache : public QObject
{ Q_OBJECT
	public:
		virtual ~NetCache() {};

		virtual void connect(const std::string& strHost, const std::string& strPort, 
			const std::string& strUser, const std::string& strPass) = 0;
		virtual void disconnect() = 0;

		virtual void refresh() = 0;

	signals:
		void connected(const QString& strHost, const QString& strSrv);
		void disconnected();

		void vars_changed(const CrystalOptions& crys, const TriangleOptions& triag);

		void updated_cache_value(const std::string& strKey, const CacheVal& val);
		void cleared_cache();
};

#endif
