/*
Copyright 2021 Maurice Dupont

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

const express = require("express");
const connection = require("../models/db.js");

const router = express.Router();

router.get('/putzplan', (req, res) => {
    var sql = 'SELECT Name, erledigt FROM putzplan.plan;';

    connection.query((sql), (err, rows) => {
        console.log(rows);
        res.json(rows)
    });
});

router.post('/rotate', (req, res) => {
    var sql = `UPDATE putzplan.plan SET Name='${req.body.name}', erledigt=FALSE WHERE id=${req.body.id};`;

    connection.query((sql), (err, rows) => {
        if(err) res.send("Update not successfull");
        else res.send("Update successfull");
    });
});

router.post('/done', (req, res) => {
    var sql = `UPDATE putzplan.plan SET erledigt=TRUE WHERE id=${req.body.id};`;

    connection.query((sql), (err, rows) => {
        if(err) res.send("Update not successfull");
        else res.send("Update successfull");
    });
});

module.exports = router;
