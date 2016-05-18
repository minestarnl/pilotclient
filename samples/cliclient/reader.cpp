/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file
//! \ingroup samplecliclient

#include "reader.h"

#include <stdio.h>
#include <QByteArray>
#include <QFile>
#include <QFlags>
#include <QIODevice>
#include <QtGlobal>

namespace BlackSample
{
    void LineReader::run()
    {
        QFile file;
        file.open(stdin, QIODevice::ReadOnly | QIODevice::Text);
        forever
        {
            QString line = file.readLine().trimmed();
            if (! line.isEmpty())
            {
                emit command(line);
            }
        }
    }
}
