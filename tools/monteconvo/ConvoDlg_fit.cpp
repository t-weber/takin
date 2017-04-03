/**
 * monte carlo convolution tool -> convolution fitting
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2015, 2016
 * @license GPLv2
 */

#include "ConvoDlg.h"
#include <QMessageBox>

using t_real = t_real_reso;


/**
 * TODO: start 1d or 2d convolution fits
 */
void ConvoDlg::StartFit()
{
	QMessageBox::information(this, "Info", "Fitting is not yet supported in this GUI program.\n"
		"Please use \"File\" -> \"Export to Convofit...\" and run the \"convofit\" command-line tool instead.");
}
