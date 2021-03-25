const express = require( 'express' ); // import express package
const bodyParser = require( 'body-parser' ); // use body parser
const path = require( 'path' );
const fs = require( 'fs' );
const app = express(); // define the main app as the express package
const port = 9000;

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


app.post('/submit', ( req, res ) => {
  // get data from the form and save it to fields as a string
  fields = JSON.stringify( req.body, null, 2 );

  // write fields to file
  fs.writeFile( 'cloud_adapters/MQTTConfig-working.json', fields, ( err ) => {
    if ( err ) {
      throw err;
    }
    console.log( "JSON data was saved." );
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
  fs.readFile( 'cloud_adapters/MQTTConfig-working.json', 'utf-8', ( err, data ) => {
    if ( err ) {
      throw err;
    }

    // check if test.json is empty, if it's empty, load in the default
    if ( data === "" ) {
      // loading in default json
      fs.readFile( 'cloud_adapters/MQTTConfig-default.json', 'utf-8', ( err, data ) => {
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
