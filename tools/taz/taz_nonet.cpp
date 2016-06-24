/*
 * TAS tool (dummy server stuff)
 * @author tweber
 * @date mar-2015
 * @copyright GPLv2
 */

#include "taz.h"

void TazDlg::ShowConnectDlg() {}
void TazDlg::ConnectTo(int iSys, const QString& _strHost, const QString& _strPort, const QString& _strUser, const QString& _strPass) {}
void TazDlg::Disconnect() {}
void TazDlg::ShowNetCache() {}
void TazDlg::NetRefresh() {}
void TazDlg::Connected(const QString& strHost, const QString& strSrv) {}
void TazDlg::Disconnected() {}
void TazDlg::VarsChanged(const CrystalOptions& crys, const TriangleOptions& triag) {}
