# ambryfs
Ambryfs is a filesystem implemented in fuse and backed by Ambry. It currently only supports read and delete operations.

Disclaimer
===========

This is a purely educational project. You can modify, improve and redistribute it under the license from the LICENSE file. Questions, support requests or kudos to tofig at freebsd dot az

Compiling
===========

Fuse library is available on plethora of platforms. You need to have it installed along with other standard development toolchain on your OS of choice. Adjust the Makefile and run make.

Running
===========

Once compiled run:


    ./ambryfs -s --ambry_base_url="http://localhost"  --ambry_port=1174 depot/

Where

    -s run fuse in single threaded mode (multithreaded mode is not supported in this revision)
    --ambry_base_url is the endpoint address for your Ambry frontend
    --ambry_port is the port of your Ambry frontend
    depot is the mountpoint of your filesystem

Typical use case
==================

Given a known Ambry Blob Id you can cat it using a standard cat command:


    $ # Assuming your fs is mounted into "depot" directory
    $cat depot/AAEAAQAAAAAAAAAAAAAAJDdiZGQxODYyLTQzMGEtNDA5NC1hZTcxLTY4ZGE4NGMyMDE1Ng
    this is a test content
    $ # This command just triggered an Ambry request and pulled the blob in

Additional References
==============
Ambry is Linkedin's scalable and immutable blob storage: https://github.com/linkedin/ambry.

Linkedin's engineering blog post about Ambry internals: https://engineering.linkedin.com/blog/2016/05/introducing-and-open-sourcing-ambry---linkedins-new-distributed-
