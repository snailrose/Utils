/*
-------------------------------------------------------------------------------

    Copyright (c) Charles Carley.

    Contributor(s): none yet.

-------------------------------------------------------------------------------
  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------
*/
#include "skCommandLineParser.h"
#include "Utils/skDebugger.h"
#include "Utils/skLogger.h"

using namespace skCommandLine;

Parser::Parser() :
    m_base(0),
    m_maxHelp(0),
    m_required(0),
    m_used(0)
{
}

Parser::~Parser()
{
    Options::Iterator it = m_options.iterator();
    while (it.hasMoreElements())
        delete it.getNext();
}

int Parser::getBaseName(const char *input)
{
    int offs = 0;
    if (input)
    {
        const int len = (int)skStringUtils::length(input);

        int i;
        for (i = len - 1; i >= 0 && offs == 0; --i)
            if (input[i] == '/' || input[i] == '\\')
                offs = i + 1;
    }
    return offs;
}

bool Parser::hasSwitch(const skString &sw) const
{
    return m_switches.find(sw) != SK_NPOS;
}

int Parser::parse(int argc, char **argv)
{
    m_program.clear();
    m_program.append(argv[0]);
    m_base = getBaseName(argv[0]);
    m_used = 0;
    m_scanner.clear();

    // it's easier to parse it as a single string
    for (int i = 1; i < argc; ++i)
        m_scanner.append(argv[i]);

    Token a;
    while (a.getType() != TOK_EOS)
    {
        m_scanner.lex(a);
        const int type = a.getType();

        if (a.getType() == TOK_ERROR)
        {
            skLogf(LD_ERROR, "\nunknown error %s\n\n", a.getValue().c_str());
            usage();
            return -1;
        }

        if (type == TOK_SWITCH_SHORT || type == TOK_SWITCH_LONG)
        {
            // grab the next token
            m_scanner.lex(a);

            if (a.getType() == TOK_NEXT || a.getType() == TOK_EOS)
            {
                skLogf(LD_ERROR, "expected a switch value to follow '-'\n");
                usage();
                return -1;
            }
            if (a.getValue().equals("help") || a.getValue().equals("h"))
            {
                usage();
                return -1;
            }

            // > next && < TOK_EOS
            const SKsize pos = m_switches.find(a.getValue());
            if (pos == SK_NPOS)
            {
                skLogf(LD_ERROR, "Unknown option '%s'\n", a.getValue().c_str());
                usage();
                return -1;
            }

            ParseOption *opt = m_switches.at(pos);
            opt->makePresent();

            if (!opt->isOptional())
                m_used++;

            if (opt->getArgCount() > 0)
            {
                // parse
                SKsize i;
                for (i = 0; i < opt->getArgCount(); ++i)
                {
                    m_scanner.lex(a);
                    if (a.getValue().empty())
                    {
                        skLogf(LD_ERROR,
                               "missing argument for option '%s'\n",
                               opt->getSwitch().name);
                        usage();
                        return -1;
                    }

                    opt->setValue(i, a.getValue());
                }

                if (i != opt->getArgCount())
                {
                    skLogf(LD_ERROR,
                           "not all arguments converted when parsing switch '%s'\n",
                           opt->getSwitch().name);
                    usage();
                    return -1;
                }
            }
        }
        else if (type >= TOK_OPTION && type < TOK_EOS)
            m_argList.push_back(a.getValue());
        else
        {
            if (type != TOK_EOS)
            {
                skLogf(LD_ERROR, "unknown option '%s'\n", a.getValue().c_str());
                usage();
                return -1;
            }
        }
    }

    if (m_used != m_required)
    {
        skLogf(LD_ERROR, "missing required options.\n");
        usage();
        return -1;
    }
    return 0;
}

void Parser::logInput()
{
    skLogf(LD_INFO, "\n ~> %s %s\n\n", getBaseProgram().c_str(), m_scanner.getValue().c_str());
}

const skString &Parser::getProgram() const
{
    return m_program;
}

skString Parser::getBaseProgram() const
{
    skString returnValue;
    m_program.substr(returnValue, m_base, m_program.size());
    return returnValue;
}

void Parser::addOption(const Switch &sw)
{
    if (sw.shortSwitch != 0)
    {
        if (hasSwitch(skString(sw.shortSwitch, 1)))
        {
            skLogf(LD_WARN, "Duplicate switch '-%c' found for %s\n", sw.shortSwitch, sw.name);
            return;
        }
    }

    if (sw.longSwitch != nullptr)
    {
        const skString lsw = skString(sw.longSwitch);

        m_maxHelp = skMax(m_maxHelp, (int)lsw.size());

        if (hasSwitch(lsw))
        {
            skLogf(LD_WARN, "Duplicate switch '--%l' found for %s\n", sw.longSwitch, sw.name);
            return;
        }
    }

    if (sw.name != nullptr)
    {
        if (hasSwitch(skString(sw.name)))
        {
            skLogf(LD_WARN, "Duplicate switch for %s\n", sw.name);
            return;
        }
    }

    auto *opt = new ParseOption(sw);
    m_options.push_back(opt);

    if (sw.shortSwitch != 0)
        m_switches.insert(skString(sw.shortSwitch, 1), opt);
    if (sw.longSwitch != nullptr)
        m_switches.insert(sw.longSwitch, opt);
    if (sw.name != nullptr)
        m_switches.insert(sw.name, opt);
}

bool Parser::isPresent(const skString &name)
{
    const SKsize pos = m_switches.find(name);
    if (pos != SK_NPOS)
    {
        ParseOption *opt = m_switches.at(pos);
        if (opt)
            return opt->isPresent();
    }
    return false;
}

ParseOption *Parser::getOption(const skString &name)
{
    ParseOption *ret = nullptr;
    const SKsize pos = m_switches.find(name);
    if (pos != SK_NPOS)
        ret = m_switches.at(pos);
    return ret;
}

void Parser::usage()
{
    skDebugger::writeColor(CS_DARKYELLOW);
    logInput();
    skDebugger::writeColor(CS_WHITE);

    skLogf(LD_INFO, "Usage: %s <options>\n\n", getBaseProgram().c_str());
    skLogf(LD_INFO, "  <options>: ");
    skLogf(LD_INFO, "<!> == required\n\n");
    skLogf(LD_INFO, "    -h, --help");

    for (int i = 0; i < m_maxHelp - 4; i++)
        skLogf(LD_INFO, " ");

    skLogf(LD_INFO, " Display this help message.\n");

    Options::Iterator it = m_options.iterator();
    while (it.hasMoreElements())
    {
        ParseOption * opt = it.getNext();
        const Switch &sw  = opt->getSwitch();

        if (!sw.optional)
            skLogf(LD_INFO, "!, ");
        else
            skLogf(LD_INFO, "    ");

        if (sw.shortSwitch != 0)
        {
            skLogf(LD_INFO, "-%c", sw.shortSwitch);
            if (sw.longSwitch != nullptr)
                skLogf(LD_INFO, ", ");
            else
                skLogf(LD_INFO, "  ");
        }
        else
            skLogf(LD_INFO, "    ");

        int space = m_maxHelp;
        if (sw.longSwitch != nullptr)
        {
            space -= (int)skStringUtils::length(sw.longSwitch);
            skLogf(LD_INFO, "--%s", sw.longSwitch);
        }

        for (int i = 0; i < space; i++)
            skLogf(LD_INFO, " ");

        skDebugger::writeColor(CS_LIGHT_GREY);

        if (sw.description != nullptr)
            skLogf(LD_INFO, " %s", sw.description);
        skDebugger::writeColor(CS_WHITE);
        skLogf(LD_INFO, "\n");
    }

    skLogf(LD_INFO, "\n");
}

void Parser::initializeSwitches(const Switch *switches, SKsize count)
{
    for (SKsize i = 0; i < count; ++i)
    {
        if (!switches[i].optional)
            m_required++;

        addOption(switches[i]);
    }
}
