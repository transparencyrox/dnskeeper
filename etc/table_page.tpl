<html>
    <head>
    <link rel="stylesheet" type="text/css" href="dnskeeper.css" />

    <title>
    <!-- TITLE -->
    </title>

    <script> 
        function async_once(divtag, url) {
            fetch(url)
                .then(response => response)
                .then(data => { 
                    console.log('Status:', data); 
                    location.reload();
                })
                .catch((error) => { 
                    console.error('Error:', error); 
                });
        }   
    </script>
    </head>
    <body>
        <header id="hdr"></header>
        <article id="content">
        <H2>
        <!-- TITLE -->
        </H2>
        <H4>
        <!-- SUBTITLE -->
        </H4>
        <table class="basic">
            <thead>
                <tr>
                <!-- HEADERS -->
                </tr>
            </thead>
            <tbody>
                <!-- ROWS -->
            </tbody>
        </table>
        </article>
        <nav id="leftbar"></nav>
        <div id="rightbar"></div>
        <footer id="ftr"></footer>
    </body>
</html>