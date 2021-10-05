The createTestCerts.sh script in this directory is loosely based on gencerts.sh under
jms_td_tests/chainedcerts.

Description of client cert files (for client cert authentication testing)
Client certs signed by rootCA A (these will be recognized as "valid" client certs)
1. imaclienta-crt.pem - Generic CN / No SubjectAltName
2. imaclienta2-crt.pem - Generic CN / Generic SubjectAltName email
3. imaclienta3-crt.pem - Valid CN for matching but not for client ID construction (d:dtype: ) / No SubjectAltName
4. imaclienta-crt4.pem - Valid CN for matching and for client ID construction (g:gytpe:g1 ) / No SubjectAltName
5. imaclienta-crt5.pem - Generic CN / SubjectAltName email valid for matching but not for client ID construction (g:gtype: )
6. imaclienta-crt6.pem - Generic CN / SubjectAltName email valid for matching and for client ID construction (d:dtype:d1 )
7. imaclienta-crt7.pem - Generic CN / SubjectAltName with one generic email followed by email valid for matching and for client ID construction (g:gtype:g1 )

Cert files named imaclientc* are signed by rootCA C but the descriptions (WRT CN/SubjectAltName) are the same as above.  These certs will not be recognized because the orgs for testing will use rootCA A (not C) as their CA certificate.  These certs will be used for tests where client certificate authentication should fail.  Some tests will verify that connection fails, other tests will verify that the proxy will not reject the connection pending actions taken by the authentication module.
