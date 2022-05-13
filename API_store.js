exports.setApp = function ( app, client )
{
  app.post('/api/recievefromESP32', async (req, res, next) =>
    {
      //incoming 64bit encoding of pic
      //outgoing 64bit encoding of pic

      //post commands from the esp needed

      const {buffer} = req.body;

      var error = '';

      var newBuffer = {Buffer:buffer};

      console.log(newBuffer);

      try
      {
        const db = client.db();
        const result = db.collection('CameraPics').insertOne(newBuffer);
      }
      catch(e)
      {
        console.log(e.message);
      }

      var ret = {error: error};
      
      res.status(200).json(ret);

    });

    // ************************************ ADD PIC API ******************************************************
    //
    // *******************************************************************************************************

    // API for
    app.post('/api/addPic', async (req, res, next) => 
    {
      // incoming: pic, email/userid
      // outgoing: error

      // Grabbing picture from parameter
      const {userId, pic, jwtToken} = req.body;

      // Checking to see if token has expired
      try
      {
        if( token.isExpired(jwtToken))
        {
          var r = {error:'The JWT is no longer valid', jwtToken: ''};
          res.status(200).json(r);
          return;
        }
      }
      catch(e)
      {
        console.log(e.message);
      }
      // Variable Declaration
      var newPic = {UserId:Number(userId), Pic:pic};
      var error = '';

      // Connecting to database and adding a picture
      try
      {
        const db = client.db();
        const result = await db.collection('UserPics').insertOne(newPic);
        // this script looks at all the pictures in the User Pics once a new picture has been added
        // it then removes the old encoded document and adds a new encoded document
        
        // PythonShell.run("newCreate_encoding.py", null, function(err,results){
        //   console.log(results);
        //   console.log("Python script finished");
        // })
      }
      
      // Prints error if failed
      catch(e)
      {
        console.log(e.message);
      }
      var refreshedToken = null;
      try
      {
        refreshedToken = token.refresh(jwtToken).accessToken;
      }
      catch(e)
      {
        console.log(e.message);
      }
    
      // return
      var ret = {error: error, jwtToken:refreshedToken};
      
      res.status(200).json(ret);
    });
}
