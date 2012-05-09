/**
 * @file include/widgets/CommandLineArgument.h
 * @brief simple helper class for conventiently accessing commandline from qt applications. handles
 * the usual stuff, like default value and printing "--help" information in  aunified way.
 *
 * for example, write the following cpp-code:
 *

    QString val = CommandLineArgumentParse("--nicearg", "default", "this is a helpfull message");
    qDebug() << "got:" << val;

 *
 * and then calling the application the following way will result in the shown messages::
 *

    $ ./example_app
    got: "default"

    $ ./example_app --help
    CommandLineArgument: "--nicearg"
                default: "default"
            description: "this is a helpfull message"
    got: "default"

    $ ./example_app --nicearg=bla
    got: "bla"

 *
 * @author Martin Zenzes
 * @date 2012-04-26
 */
#ifndef NDLCOM_COMMANDLINEARGUMENT_H
#define NDLCOM_COMMANDLINEARGUMENT_H

#include <QCoreApplication>
#include <QVariant>
#include <QStringList>
#include <QDebug>

namespace NDLCom
{
    static QString CommandLineArgument(const QString &argname, const QString &defaultValue, const QString &description)
    {
        QString retval = defaultValue;

        QStringList args = QCoreApplication::arguments();
        for(int i =0;i<args.size();i++)
        {
            if (args.at(i).contains(argname))
            {
                retval = args.at(i).split("=").at(1);
            }
            else if (args.at(i).contains("--help"))
            {
                qDebug() << "CommandLineArgument:" << argname << "\n"
                         << "           default:" << defaultValue << "\n"
                         << "       description:" << description;
            }
        }
        return retval;
    }
};

#endif /*NDLCOM_COMMANDLINEARGUMENT_H*/
