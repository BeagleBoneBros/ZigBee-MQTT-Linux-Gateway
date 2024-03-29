const express = require( 'express' ); // import express package
const bodyParser = require( 'body-parser' ); // use body parser
const path = require( 'path' );
const fs = require( 'fs' );
var events = require('events');
const app = express(); // define the main app as the express package
const port = 9000;

var ConfigServerInstance;

function ConfigServer(ip_address) {

  // Make sure there is only one instance created
  if (typeof ConfigServerInstance !== "undefined") {
    return ConfigServerInstance;
  }
  /* Set up to emit events */
  events.EventEmitter.call(this);
  ConfigServerInstance = this;

const workingJson = 'cloud_adapters/MQTTConfig-working.json';
const defaultJson = 'cloud_adapters/MQTTConfig-default.json';

app.set( 'view engine', 'pug' ); // set view engine to pug

// creat request body object
app.use( bodyParser.urlencoded( { extended: true } ) );
app.set( 'views', path.join(__dirname, 'views'));
app.set( 'cloud_adapters', path.join(__dirname, '../cloud_adapters'));

app.use( express.static( path.join( __dirname, 'public' ) ) );

var fields = {};
// serve the index file. root dir ('/') is views
app.get( '/', ( req, res ) => {
  res.render( 'index', fields );
} );

function isTrue( string ) {
  if ( string === 'true') {
    return true;
  } else {
    return false;
  }
};

app.post('/submit', ( req, res ) => {
  var reqBodyClean = req.body.clean;
  var isClean = isTrue( reqBodyClean );
  // get data from the form and save it to fields as a string
  fields = {
    broker_address: req.body.broker_address,
    broker_options: {
      clientId: req.body.clientId,
  		username: req.body.username,
  		password: req.body.password,
  		clean: isTrue( req.body.clean )
  	},
  	sub_options: {
  		  retain: isTrue( req.body.retain ),
  		  qos: parseInt( req.body.qos )
  	},
  };
  console.log( fields );
  fields = JSON.stringify( fields, null, 2 );
  console.log( fields );

  // write fields to file
  fs.writeFile( workingJson, fields, ( err ) => {
    if ( err ) {
      throw err;
    }else{
    console.log( "JSON data was saved -- called MQTT Adapter Reset." );
    //Restart MQTTadapter app to apply new configuration
    }
    const { spawn } = require('child_process');
    const reset = spawn('sh', ['reset.sh'], {detached : true, stdio: 'ignore'});

  } );

  // get fields back into json
  fields = JSON.parse( fields );
  console.log( fields );

  // return back to the form page
  res.redirect( '/' );
} );


// start server on port 8888
app.listen( port, () => {
  // read test.json int data
  fs.readFile( workingJson, 'utf-8', ( err, data ) => {
    if ( err ) {
      throw err;
    }

    // check if test.json is empty, if it's empty, load in the default
    if ( data === "" ) {
      // loading in default json
      fs.readFile( defaultJson, 'utf-8', ( err, data ) => {
        if ( err ) {
          throw err;
        }
        // parsing default json into fields
        fields = JSON.parse( data );
      } );
    }
    else {
      // loading test.json into fields
      fields = JSON.parse( data );
    }
  } );
});


}
ConfigServer.prototype.__proto__ = events.EventEmitter.prototype;
module.exports = ConfigServer;