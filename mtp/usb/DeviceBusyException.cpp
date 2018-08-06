#include <mtp/usb/DeviceBusyException.h>
#include <mtp/log.h>
#ifdef __linux__
#	include <fcntl.h>
#	include <signal.h>
#	include <unistd.h>
#	include <linux/limits.h>

//mtp
#	include <Exception.h>
#	include <usb/Directory.h>
#endif

namespace mtp { namespace usb
{
	namespace
	{
		std::string ReadLink(const std::string &path)
		{
#ifdef __linux__
			char buf[NAME_MAX];
			auto pathSize = readlink(path.c_str(), buf, sizeof(buf));
			if (pathSize >= 0)
				return std::string(buf, pathSize);

			debug("Readlink ", path, ": ", posix::Exception::GetErrorMessage(errno));
			return buf;
#endif
			return std::string();
		}
	}

	DeviceBusyException::DeviceBusyException(int fd, const std::string &msg):
		std::runtime_error(msg)
	{
		try
		{
//only linux for now
#ifdef __linux__
		do
		{
			if (fd < 0)
				break;

			debug("trying to find processes which are holding fd ", fd);
#if 0 //apple
			char filePath[PATH_MAX];
			if (fcntl(fd, F_GETPATH, filePath) == -1)
			{
				error("error getting file path for fd ", fd, ": ", Exception::GetErrorMessage(errno));
				break;
			}
#else
			std::string filePath = ReadLink("/proc/self/fd/" + std::to_string(fd));
			if (filePath.empty())
				break;
#endif
			debug("mapped ", fd, " to ", filePath);
			Directory proc("/proc");
			while(true)
			{
				auto pidPath = proc.Read();
				if (pidPath.empty())
					break;

				int pid;
				if (sscanf(pidPath.c_str(), "%d", &pid) != 1)
					continue;

				debug("got process dir ", pidPath);
				auto fdsRoot = "/proc/" + pidPath + "/fd";

				try
				{
					Directory fds(fdsRoot);
					while(true)
					{
						auto fdPath = fds.Read();
						if (fdPath.empty())
							break;

						auto fdTarget = ReadLink(fdsRoot + "/" + fdPath);
						if (fdTarget.empty())
							continue;

						//debug("read mapping to ", fdTarget);
						if (fdTarget == filePath)
						{
							debug("process ", pid, " is holding file descriptor to ", filePath);
							auto image = ReadLink("/proc/" + pidPath + "/exe");
							Processes.push_back({ pid, image });
						}
					}
				}
				catch(const std::exception & ex)
				{ debug("error reading ", fdsRoot, ": ", ex.what()); }
			}
		}
		while(false);
#endif
		} catch(const std::exception &ex)
		{ debug("DeviceBusyException error: ", ex.what()); }
	}

	void DeviceBusyException::Kill()
	{
		for(auto desc : Processes)
		{
			try
			{ Kill(desc); }
			catch(const std::exception & ex)
			{ error("Kill: ", ex.what()); }
		}
	}

	void DeviceBusyException::Kill(ProcessDescriptor desc)
	{
#ifdef __linux__
		if (kill(desc.Id, SIGTERM) != 0)
			throw posix::Exception("kill(" + std::to_string(desc.Id) + " (" + desc.Name + "), SIGTERM)");
		sleep(1);
		kill(desc.Id, SIGKILL); //assuming we can do it
#else
		throw std::runtime_error("not implemented");
#endif
	}


}}
