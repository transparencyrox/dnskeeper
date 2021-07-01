# DNS Keeper

This is a system that allows manipulation of 'A' records on Route53. It runs off the Heroku free tier and offers an express way to manage DNS (might be useful if you run a cloud centric or VPN company e.g.)

### Required configuration variables
- AWS_SECRET_ACCESS_KEY
- AWS_ACCESS_KEY_ID
- AWS_DEFAULT_REGION
- DOMAIN_NAME
- DATABASE_URL
### Assumptions
- The domain (and at least one hosted zone) have been setup
- We assume single instance of the app running on Heroku. There is nothing inherently present in the design that restricts scaling to multiple nodes, but it has been certified to work in single instance mode
- We assume a 1:1 mapping between the subdomain and the cluster. That is we have a subdomain of ca.pyrotechnics.com, it would have a single cluster associated with it (along with servers attached to it)
- For the purposes of this exercise, we assume all dependent libraries need to be compiled as a part of the build. This is because:
  
  1. Dependencies like the AWS SDK are not shipped as a part of standard Linux distributions
  2. C++ does not really have a regular package repository like Rust or Python, necessitating custom builds
  3. Heroku supported distributions don't appear to have proper support for some dependencies like pqxx (PostrgreSQL)
  4. Adding externally supplied repositories or storage complicates submission of the test exercise (security and stability guarantees of URLs located elsewhere e.g.)
   
### Known issues and caveats

1. The server and UI allow multiple parallel requests but the UI itself refreshes after the first reply. The manifestation of this problem is apparent if 2-3 requests are made spaced apart by about 10 seconds (the Route53 calls take a long time to finish and by the time the first call responds, the others have not updated their status as yet even though the requests are still in motion at the backend). This requires a more sophisticated design for the frontend which requires more time.
2. The unit tests take long (and have been deliberately left without timeouts) when testing Route53 calls. This is because DNS sync operations inherently take time to complete and report success or failure.
3. Build times take slightly longer than they would if this were a regular system. This is partly caused by the assumptions made above (locally built libraries), as well as inherent difficulties in integrating C++ with Heroku as stated in comments on the compile script

    ```
     Note: The build folder on Heroku gets relocated per build.
           The cache folder gets repopulated with binaries but
           changes its path, which makes linking to C++ onerous.
           We just pull the cache and do a make install. It 
           slightly lengthens build time for the app but keeps
           dependent library link paths stable for the binary.
    ```
