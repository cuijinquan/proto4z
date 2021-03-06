﻿/*
 * ZSUMMER License
 * -----------
 * 
 * ZSUMMER is licensed under the terms of the MIT license reproduced below.
 * This means that ZSUMMER is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2014-2015 YaweiZhang <yawei.zhang@foxmail.com>.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * ===============================================================================
 * 
 * (end of COPYRIGHT)
 */

#ifndef _CRT_SECURE_NO_WARNINGS
#define  _CRT_SECURE_NO_WARNINGS
#endif
#include "genProto.h"
#include <time.h>
#include <algorithm>

std::string getCPPType(std::string type)
{
	auto founder = xmlTypeToCPPType.find(type);
	if (founder != xmlTypeToCPPType.end() && !founder->second.empty())
	{
		type = founder->second;
	}
	return type;
}

bool genCppFile(std::string filename, std::vector<StoreInfo> & stores, bool genLog4z)
{
	std::string macroFileName = std::string("_") + filename  + "_H_";
	std::transform(macroFileName.begin(), macroFileName.end(), macroFileName.begin(), [](char ch){ return std::toupper(ch); });


	std::string text = LFCR + "#ifndef " + macroFileName + LFCR;
	text += "#define " + macroFileName + LFCR + LFCR;

	for (auto &info : stores)
	{
		if (info._type == GT_DataInclude)
		{
			text += "#include <" + info._include._filename + ".h> ";
			if (!info._include._desc.empty())
			{
				text += "//" + info._include._desc;
			}
			text += LFCR;
		}
		else if (info._type == GT_DataConstValue)
		{
			text += "const " + getCPPType(info._const._type) + " " + info._const._name + " = " + info._const._value + "; ";
			if (!info._const._desc.empty())
			{
				text += "//" + info._const._desc;
			}
			text += LFCR;
		}
		else if (info._type == GT_DataArray)
		{
			text += LFCR + "typedef std::vector<" + getCPPType(info._array._type) + "> " + info._array._arrayName + "; ";
			if (!info._array._desc.empty())
			{
				text += "//" + info._array._desc;
			}
			text += LFCR;
		}
		else if (info._type == GT_DataMap)
		{
			text += LFCR + "typedef std::map<"
				+ getCPPType(info._map._typeKey) + ", " + getCPPType(info._map._typeValue) 
				+ "> " + info._map._mapName + "; ";
			if (!info._map._desc.empty())
			{
				text += "//" + info._map._desc;
			}
			text += LFCR;
		}
		else if (info._type == GT_DataStruct || info._type == GT_DataProto)
		{
			text += LFCR;
			//write ProtoID
			if (info._type == GT_DataProto)
			{
				text += "const " + getCPPType(info._proto._const._type) + " " 
					+ info._proto._const._name + " = " + info._proto._const._value + "; ";
				if (!info._proto._const._desc.empty())
				{
					text += "//" + info._proto._const._desc;
				}
				text += LFCR;
			}

			//write struct
			text += "struct " + info._proto._struct._name;
			if (!info._proto._struct._desc.empty())
			{
				text += " //" + info._proto._struct._desc;
			}
			text += LFCR;
			text += "{" + LFCR;

			info._proto._struct._tag = 0;
			int curTagIndex = 0;
			for (const auto & m : info._proto._struct._members)
			{
				{
					if (!m._isDel)
					{
						info._proto._struct._tag |= (1ULL << curTagIndex);
					}
					curTagIndex++;
				}
				
				text += "\t" + getCPPType(m._type) + " " + m._name + "; ";
				if (m._isDel)
				{
					text += "//[already deleted]";
				}
				if (!m._desc.empty())
				{
					text += "//" + m._desc;
				}
				text += LFCR;
			}

			{	//struct init
				std::string defaltInit = "\t" + info._proto._struct._name + "()" + LFCR;
				defaltInit += "\t{" + LFCR;
				std::string memberText;
				for (const auto &m : info._proto._struct._members)
				{
					auto founder = xmlTypeToCPPDefaultValue.find(m._type);
					if (founder != xmlTypeToCPPDefaultValue.end() && !founder->second.empty())
					{
						memberText += "\t\t" + m._name + " = " + founder->second + ";" + LFCR;
					}
				}
				if (!memberText.empty())
				{
					text += defaltInit;
					text += memberText;
					text += "\t}" + LFCR;
				}
			}
			if (info._type == GT_DataProto)
			{
				text += std::string("\tinline ") + getCPPType(ProtoIDType) + " GetProtoID() { return " + info._proto._const._value + ";}" + LFCR;
				text += std::string("\tinline ") + getCPPType("string") + " GetProtoName() { return \""
					+ info._proto._const._name + "\";}" + LFCR;
			}
			text += "};" + LFCR;


			//input stream operator
			text += "inline zsummer::proto4z::WriteStream & operator << (zsummer::proto4z::WriteStream & ws, const " + info._proto._struct._name + " & data)" + LFCR;
			text += "{" + LFCR;
			text += "\tunsigned long long tag = " + boost::lexical_cast<std::string, unsigned long long>(info._proto._struct._tag) + "ULL;" + LFCR;
			text += "\tws << (zsummer::proto4z::Integer)0;" + LFCR;
			text += "\tzsummer::proto4z::Integer offset = ws.getStreamLen();" + LFCR;
			text += "\tws << tag;" + LFCR;
			for (const auto &m : info._proto._struct._members)
			{
				if (m._isDel)
				{
					text += "//\tws << data." + m._name + "; //[already deleted]" + LFCR;
				}
				else
				{
					text += "\tws << data." + m._name + ";" + LFCR;
				}
			}
			text += "\tws.fixOriginalData(offset - 4, ws.getStreamLen() - offset);" + LFCR;
			text += "\treturn ws;" + LFCR;
			text += "}" + LFCR;


			//output stream operator
			text += "inline zsummer::proto4z::ReadStream & operator >> (zsummer::proto4z::ReadStream & rs, " + info._proto._struct._name + " & data)" + LFCR;
			text += "{" + LFCR;
			text += "\tzsummer::proto4z::Integer sttLen = 0;" + LFCR;
			text += "\trs >> sttLen;" + LFCR;
			text += "\tzsummer::proto4z::Integer cursor = rs.getStreamUnreadLen();" + LFCR;
			text += "\tunsigned long long tag = 0;" + LFCR;
			text += "\trs >> tag;" + LFCR;
			curTagIndex = 0;
			for (const auto &m : info._proto._struct._members)
			{
				text += "\tif ( (1ULL << " + boost::lexical_cast<std::string, int>(curTagIndex)+") & tag)" + LFCR;
				text += "\t{" + LFCR;
				text += "\t\trs >> data." + m._name + "; " + LFCR;
				text += "\t}" + LFCR;
				curTagIndex++;
			}
			text += "\tcursor = cursor - rs.getStreamUnreadLen();" + LFCR;
			text += "\trs.skipOriginalData(sttLen - cursor);" + LFCR;
			text += "\treturn rs;" + LFCR;
			text += "}" + LFCR;

			//input log4z operator
			if (genLog4z)
			{
				text += "inline zsummer::log4z::Log4zStream & operator << (zsummer::log4z::Log4zStream & stm, const " + info._proto._struct._name + " & info)" + LFCR;
				text += "{" + LFCR;
				bool bFirst = true;
				for (const auto &m : info._proto._struct._members)
				{
					if (bFirst)
					{
						bFirst = false;
						text += "\tstm << \"" + m._name + "=\"" + " << info." + m._name;
					}
					else
					{
						text += " << \", " + m._name + "=\"" + " << info." + m._name;
					}
				}
				text += ";" + LFCR;
				text += "\treturn stm;" + LFCR;
				text += "}" + LFCR;
			}
		}

	}
	text += LFCR + "#endif" + LFCR;

	std::ofstream os;
	os.open(getCPPFile(filename), std::ios::binary);
	if (!os.is_open())
	{
		LOGE("genCppFile open file Error. : " << getCPPFile(filename));
		return false;
	}
	os.write(text.c_str(), text.length());
	os.close();
	return true;
}
