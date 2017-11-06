/**
 * Convolution fitting
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date dec-2015
 * @license GPLv2
 */

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/scope_exit.hpp>
#include <boost/program_options.hpp>

#include "convofit.h"
#include "libs/version.h"
#include "libs/globals.h"
#include "tlibs/time/stopwatch.h"
#include "tlibs/helper/thread.h"

namespace asio = boost::asio;
namespace sys = boost::system;
namespace opts = boost::program_options;

using t_real = t_real_reso;


// ----------------------------------------------------------------------------
// main program

template<class T>
static inline void get_prog_option(opts::variables_map& map, const char* pcKey, T& var)
{
	if(map.count(pcKey))
		var = map[pcKey].as<T>();
}


int main(int argc, char** argv)
{
	try
	{
	#ifdef NO_TERM_CMDS
		tl::Log::SetUseTermCmds(0);
	#endif

		// plain C locale
		/*std::*/setlocale(LC_ALL, "C");
		std::locale::global(std::locale::classic());


		// --------------------------------------------------------------------
		// install exit signal handlers
		asio::io_service ioSrv;
		asio::signal_set sigInt(ioSrv, SIGABRT, SIGTERM, SIGINT);
		sigInt.async_wait([&ioSrv](const sys::error_code& err, int iSig)
		{
			tl::log_warn("Hard exit requested via signal ", iSig, ". This may cause a fault.");
			if(err) tl::log_err("Error: ", err.message(), ", error category: ", err.category().name(), ".");
			ioSrv.stop();
	#ifdef SIGKILL
			// TODO: use specific PIDs
			//std::system("killall -s KILL gnuplot");
			std::raise(SIGKILL);
	#endif
			exit(-1);
		});
		std::thread thSig([&ioSrv]() { ioSrv.run(); });
		BOOST_SCOPE_EXIT(&ioSrv, &thSig)
		{
			//tl::log_debug("Exiting...");
			ioSrv.stop();
			thSig.join();
		}
		BOOST_SCOPE_EXIT_END
		// --------------------------------------------------------------------


		tl::log_info("This is the Takin command-line convolution fitter, version " TAKIN_VER ".");
		tl::log_info("Written by Tobias Weber <tobias.weber@tum.de>, 2014-2017.");
		tl::log_info(TAKIN_LICENSE("Takin/Convofit"));
		tl::log_debug("Resolution calculation uses ", sizeof(t_real_reso)*8, " bit ", tl::get_typename<t_real_reso>(), "s.");
		tl::log_debug("Fitting uses ", sizeof(tl::t_real_min)*8, " bit ", tl::get_typename<tl::t_real_min>(), "s.");


		// --------------------------------------------------------------------
		// get job files and program options
		std::vector<std::string> vecJobs;

		// normal args
		opts::options_description args("convofit options (overriding job file settings)");
		args.add(boost::shared_ptr<opts::option_description>(
			new opts::option_description("job-file",
			opts::value<decltype(vecJobs)>(&vecJobs),
			"convolution fitting job file")));
		args.add(boost::shared_ptr<opts::option_description>(
			new opts::option_description("neutrons",
			opts::value<decltype(g_iNumNeutrons)>(&g_iNumNeutrons),
			"neutron count")));
		args.add(boost::shared_ptr<opts::option_description>(
			new opts::option_description("skip-fit",
			opts::bool_switch(&g_bSkipFit),
			"skip the fitting step")));
		args.add(boost::shared_ptr<opts::option_description>(
			new opts::option_description("keep-model",
			opts::bool_switch(&g_bUseValuesFromModel),
			"keep the initial values from the model file")));
		args.add(boost::shared_ptr<opts::option_description>(
			new opts::option_description("model-params",
			opts::value<decltype(g_strSetParams)>(&g_strSetParams),
			"set S(q,w) model parameters")));
		args.add(boost::shared_ptr<opts::option_description>(
			new opts::option_description("outfile-suffix",
			opts::value<decltype(g_strOutFileSuffix)>(&g_strOutFileSuffix),
			"suffix to append to output files")));
		args.add(boost::shared_ptr<opts::option_description>(
			new opts::option_description("plot-points",
			opts::value<decltype(g_iPlotPoints)>(&g_iPlotSkipBegin),
			"number of plot points")));
		args.add(boost::shared_ptr<opts::option_description>(
			new opts::option_description("plot-skip-begin",
			opts::value<decltype(g_iPlotSkipBegin)>(&g_iPlotSkipBegin),
			"skip plot points in the beginning of the range")));
		args.add(boost::shared_ptr<opts::option_description>(
			new opts::option_description("plot-skip-end",
			opts::value<decltype(g_iPlotSkipEnd)>(&g_iPlotSkipEnd),
			"skip plot points in the end of the range")));
		args.add(boost::shared_ptr<opts::option_description>(
			new opts::option_description("max-threads",
			opts::value<decltype(g_iMaxThreads)>(&g_iMaxThreads),
			"maximum number of threads")));


		// positional args
		opts::positional_options_description args_pos;
		args_pos.add("job-file", -1);

		opts::basic_command_line_parser<char> clparser(argc, argv);
		clparser.options(args);
		clparser.positional(args_pos);
		opts::basic_parsed_options<char> parsedopts = clparser.run();

		opts::variables_map opts_map;
		opts::store(parsedopts, opts_map);
		opts::notify(opts_map);

		//get_prog_option<decltype(vecJobs)>(opts_map, "job-file", vecJobs);


		if(vecJobs.size() >= 2)
		{
			for(tl::Log* log : { &tl::log_info, &tl::log_warn, &tl::log_err, &tl::log_crit, &tl::log_debug })
				log->SetShowThread(1);
		}

		if(argc <= 1)
		{
			std::ostringstream ostrHelp;
			ostrHelp << "Usage: " << argv[0] << " [options] <job-file 1> <job-file 2> ...\n";
			ostrHelp << args;
			tl::log_info(ostrHelp.str());
			return -1;
		}

		if(vecJobs.size() == 0)
		{
			tl::log_err("No job files given.");
			return -1;
		}
		// --------------------------------------------------------------------


		unsigned int iNumThreads = get_max_threads();
		tl::ThreadPool<bool()> tp(iNumThreads);

		for(std::size_t iJob=0; iJob<vecJobs.size(); ++iJob)
		{
			const std::string& strJob = vecJobs[iJob];
			tp.AddTask([iJob, strJob]() -> bool
			{
				tl::log_info("Executing job file ", iJob+1, ": \"", strJob, "\".");

				Convofit convo;
				return convo.run_job(strJob);
				//if(argc > 2) tl::log_info("================================================================================");
			});
		}

		tl::Stopwatch<t_real> watch;
		watch.start();
		tp.StartTasks();

		auto& lstFut = tp.GetFutures();
		std::size_t iTask = 0;
		for(auto& fut : lstFut)
		{
			bool bOk = fut.get();
			if(!bOk)
				tl::log_err("Job ", iTask+1, " (", vecJobs[iTask], ") failed or fit invalid!");
			++iTask;
		}

		watch.stop();
		tl::log_info("================================================================================");
		tl::log_info("Start time:     ", watch.GetStartTimeStr());
		tl::log_info("Stop time:      ", watch.GetStopTimeStr());
		tl::log_info("Execution time: ", tl::get_duration_str_secs<t_real>(watch.GetDur()));
		tl::log_info("================================================================================");
	}
	catch(const std::exception& ex)
	{
		tl::log_crit(ex.what());
	}

	return 0;
}
// ----------------------------------------------------------------------------
