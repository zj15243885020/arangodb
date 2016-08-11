// Copyright (c) 2012 Ecma International.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
es5id: 15.2.3.7-5-b-56
description: >
    Object.defineProperties - value of 'enumerable' property of
    'descObj' is the global object (8.10.5 step 3.b)
includes: [fnGlobalObject.js]
---*/

        var obj = {};
        var accessed = false;

        Object.defineProperties(obj, {
            prop: {
                enumerable: fnGlobalObject()
            }
        });
        for (var property in obj) {
            if (property === "prop") {
                accessed = true;
            }
        }

assert(accessed, 'accessed !== true');