/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef AFTL_FUSE_FUSEENTRY_H
#define AFTL_FUSE_FUSEENTRY_H

#include <mtp/ptp/ObjectFormat.h>

#include <fuse/Exception.h>
#include <fuse/FuseId.h>

#include <fuse_lowlevel.h>

namespace mtp { namespace fuse
{

	struct FuseEntry : fuse_entry_param
	{
		static constexpr const double	Timeout = 60.0;
		static constexpr unsigned 		FileMode 		= S_IFREG | 0644;
		static constexpr unsigned 		DirectoryMode	= S_IFDIR | 0755;

		fuse_req_t Request;

		FuseEntry(fuse_req_t req): fuse_entry_param(), Request(req)
		{
			generation = 1;
			attr_timeout = entry_timeout = Timeout;
		};

		void SetId(FuseId id)
		{
			ino = id.Inode;
			attr.st_ino = id.Inode;
		}

		void SetFileMode()
		{ attr.st_mode = FileMode; }

		void SetDirectoryMode()
		{ attr.st_mode = DirectoryMode; }

		static mode_t GetMode(mtp::ObjectFormat format)
		{
			switch(format)
			{
			case mtp::ObjectFormat::Association:
				return DirectoryMode;
			default:
				return FileMode;
			}
		}

		void Reply()
		{
			if (attr.st_mode == 0)
				throw std::runtime_error("uninitialized attr in FuseEntry::Reply");
			FUSE_CALL(fuse_reply_entry(Request, this));
		}

		void ReplyCreate(fuse_file_info *fi)
		{
			if (attr.st_mode == 0)
				throw std::runtime_error("uninitialized attr in FuseEntry::Reply");
			FUSE_CALL(fuse_reply_create(Request, this, fi));
		}

		void ReplyAttr()
		{
			if (attr.st_mode == 0)
				throw std::runtime_error("uninitialized attr in FuseEntry::ReplyAttr");
			FUSE_CALL(fuse_reply_attr(Request, &attr, Timeout));
		}

		void ReplyError(int err)
		{
			FUSE_CALL(fuse_reply_err(Request, err));
		}
	};

}}

#endif
