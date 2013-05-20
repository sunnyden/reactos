/*
 * Wininet - networking layer. Uses unix sockets or OpenSSL.
 *
 * Copyright 2002 TransGaming Technologies Inc.
 *
 * David Hammerton
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <config.h>
//#include "wine/port.h"

#define NONAMELESSUNION

#include <sys/types.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#include <time.h>
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif
#ifdef HAVE_OPENSSL_SSL_H
# include <openssl/ssl.h>
# include <openssl/opensslv.h>
#undef FAR
#undef DSA
#endif

#include <stdarg.h>
//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>
//#include <errno.h>
#include <assert.h>

//#include "wine/library.h"
#include <windef.h>
#include <winbase.h>
#include <wininet.h>
//#include "winerror.h"
//#include "wincrypt.h"

#if defined(__MINGW32__) || defined (_MSC_VER)
#include <ws2tcpip.h>
#endif

#include <wine/debug.h>
#include "internet.h"

/* To avoid conflicts with the Unix socket headers. we only need it for
 * the error codes anyway. */
#define USE_WS_PREFIX
//#include "winsock2.h"

#define RESPONSE_TIMEOUT        30            /* FROM internet.c */


WINE_DEFAULT_DEBUG_CHANNEL(wininet);

/* FIXME!!!!!!
 *    This should use winsock - To use winsock the functions will have to change a bit
 *        as they are designed for unix sockets.
 *    SSL stuff should use crypt32.dll
 */

#ifdef SONAME_LIBSSL

#include <openssl/err.h>

static void *OpenSSL_ssl_handle;
static void *OpenSSL_crypto_handle;

#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER > 0x10000000)
static const SSL_METHOD *meth;
#else
static SSL_METHOD *meth;
#endif
static SSL_CTX *ctx;
static int error_idx;
static int conn_idx;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f

/* OpenSSL functions that we use */
MAKE_FUNCPTR(SSL_library_init);
MAKE_FUNCPTR(SSL_load_error_strings);
MAKE_FUNCPTR(SSLv23_method);
MAKE_FUNCPTR(SSL_CTX_free);
MAKE_FUNCPTR(SSL_CTX_new);
MAKE_FUNCPTR(SSL_CTX_ctrl);
MAKE_FUNCPTR(SSL_new);
MAKE_FUNCPTR(SSL_free);
MAKE_FUNCPTR(SSL_ctrl);
MAKE_FUNCPTR(SSL_set_fd);
MAKE_FUNCPTR(SSL_connect);
MAKE_FUNCPTR(SSL_shutdown);
MAKE_FUNCPTR(SSL_write);
MAKE_FUNCPTR(SSL_read);
MAKE_FUNCPTR(SSL_pending);
MAKE_FUNCPTR(SSL_get_error);
MAKE_FUNCPTR(SSL_get_ex_new_index);
MAKE_FUNCPTR(SSL_get_ex_data);
MAKE_FUNCPTR(SSL_set_ex_data);
MAKE_FUNCPTR(SSL_get_ex_data_X509_STORE_CTX_idx);
MAKE_FUNCPTR(SSL_get_peer_certificate);
MAKE_FUNCPTR(SSL_CTX_get_timeout);
MAKE_FUNCPTR(SSL_CTX_set_timeout);
MAKE_FUNCPTR(SSL_CTX_set_default_verify_paths);
MAKE_FUNCPTR(SSL_CTX_set_verify);
MAKE_FUNCPTR(SSL_get_current_cipher);
MAKE_FUNCPTR(SSL_CIPHER_get_bits);

/* OpenSSL's libcrypto functions that we use */
MAKE_FUNCPTR(BIO_new_fp);
MAKE_FUNCPTR(CRYPTO_num_locks);
MAKE_FUNCPTR(CRYPTO_set_id_callback);
MAKE_FUNCPTR(CRYPTO_set_locking_callback);
MAKE_FUNCPTR(ERR_free_strings);
MAKE_FUNCPTR(ERR_get_error);
MAKE_FUNCPTR(ERR_error_string);
MAKE_FUNCPTR(X509_STORE_CTX_get_ex_data);
MAKE_FUNCPTR(X509_STORE_CTX_get_chain);
MAKE_FUNCPTR(i2d_X509);
MAKE_FUNCPTR(sk_num);
MAKE_FUNCPTR(sk_value);
#undef MAKE_FUNCPTR

static CRITICAL_SECTION *ssl_locks;
static unsigned int num_ssl_locks;

static unsigned long ssl_thread_id(void)
{
    return GetCurrentThreadId();
}

static void ssl_lock_callback(int mode, int type, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
        EnterCriticalSection(&ssl_locks[type]);
    else
        LeaveCriticalSection(&ssl_locks[type]);
}

static PCCERT_CONTEXT X509_to_cert_context(X509 *cert)
{
    unsigned char* buffer,*p;
    INT len;
    BOOL malloced = FALSE;
    PCCERT_CONTEXT ret;

    p = NULL;
    len = pi2d_X509(cert,&p);
    /*
     * SSL 0.9.7 and above malloc the buffer if it is null.
     * however earlier version do not and so we would need to alloc the buffer.
     *
     * see the i2d_X509 man page for more details.
     */
    if (!p)
    {
        buffer = heap_alloc(len);
        p = buffer;
        len = pi2d_X509(cert,&p);
    }
    else
    {
        buffer = p;
        malloced = TRUE;
    }

    ret = CertCreateCertificateContext(X509_ASN_ENCODING,buffer,len);

    if (malloced)
        free(buffer);
    else
        heap_free(buffer);

    return ret;
}

static DWORD netconn_verify_cert(netconn_t *conn, PCCERT_CONTEXT cert, HCERTSTORE store)
{
    BOOL ret;
    CERT_CHAIN_PARA chainPara = { sizeof(chainPara), { 0 } };
    PCCERT_CHAIN_CONTEXT chain;
    char oid_server_auth[] = szOID_PKIX_KP_SERVER_AUTH;
    char *server_auth[] = { oid_server_auth };
    DWORD err = ERROR_SUCCESS, errors;

    static const DWORD supportedErrors =
        CERT_TRUST_IS_NOT_TIME_VALID |
        CERT_TRUST_IS_UNTRUSTED_ROOT |
        CERT_TRUST_IS_PARTIAL_CHAIN |
        CERT_TRUST_IS_NOT_VALID_FOR_USAGE;

    TRACE("verifying %s\n", debugstr_w(conn->server->name));

    chainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
    chainPara.RequestedUsage.Usage.rgpszUsageIdentifier = server_auth;
    if (!(ret = CertGetCertificateChain(NULL, cert, NULL, store, &chainPara, 0, NULL, &chain))) {
        TRACE("failed\n");
        return GetLastError();
    }

    errors = chain->TrustStatus.dwErrorStatus;

    do {
        /* This seems strange, but that's what tests show */
        if(errors & CERT_TRUST_IS_PARTIAL_CHAIN) {
            WARN("ERROR_INTERNET_SEC_CERT_REV_FAILED\n");
            err = ERROR_INTERNET_SEC_CERT_REV_FAILED;
            if(conn->mask_errors)
                conn->security_flags |= _SECURITY_FLAG_CERT_REV_FAILED;
            if(!(conn->security_flags & SECURITY_FLAG_IGNORE_REVOCATION))
                break;
        }

        if (chain->TrustStatus.dwErrorStatus & ~supportedErrors) {
            WARN("error status %x\n", chain->TrustStatus.dwErrorStatus & ~supportedErrors);
            err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_SEC_INVALID_CERT;
            errors &= supportedErrors;
            if(!conn->mask_errors)
                break;
            WARN("unknown error flags\n");
        }

        if(errors & CERT_TRUST_IS_NOT_TIME_VALID) {
            WARN("CERT_TRUST_IS_NOT_TIME_VALID\n");
            if(!(conn->security_flags & SECURITY_FLAG_IGNORE_CERT_DATE_INVALID)) {
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_SEC_CERT_DATE_INVALID;
                if(!conn->mask_errors)
                    break;
                conn->security_flags |= _SECURITY_FLAG_CERT_INVALID_DATE;
            }
            errors &= ~CERT_TRUST_IS_NOT_TIME_VALID;
        }

        if(errors & CERT_TRUST_IS_UNTRUSTED_ROOT) {
            WARN("CERT_TRUST_IS_UNTRUSTED_ROOT\n");
            if(!(conn->security_flags & SECURITY_FLAG_IGNORE_UNKNOWN_CA)) {
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_INVALID_CA;
                if(!conn->mask_errors)
                    break;
                conn->security_flags |= _SECURITY_FLAG_CERT_INVALID_CA;
            }
            errors &= ~CERT_TRUST_IS_UNTRUSTED_ROOT;
        }

        if(errors & CERT_TRUST_IS_PARTIAL_CHAIN) {
            WARN("CERT_TRUST_IS_PARTIAL_CHAIN\n");
            if(!(conn->security_flags & SECURITY_FLAG_IGNORE_UNKNOWN_CA)) {
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_INVALID_CA;
                if(!conn->mask_errors)
                    break;
                conn->security_flags |= _SECURITY_FLAG_CERT_INVALID_CA;
            }
            errors &= ~CERT_TRUST_IS_PARTIAL_CHAIN;
        }

        if(errors & CERT_TRUST_IS_NOT_VALID_FOR_USAGE) {
            WARN("CERT_TRUST_IS_NOT_VALID_FOR_USAGE\n");
            if(!(conn->security_flags & SECURITY_FLAG_IGNORE_WRONG_USAGE)) {
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_SEC_INVALID_CERT;
                if(!conn->mask_errors)
                    break;
                WARN("CERT_TRUST_IS_NOT_VALID_FOR_USAGE, unknown error flags\n");
            }
            errors &= ~CERT_TRUST_IS_NOT_VALID_FOR_USAGE;
        }

        if(err == ERROR_INTERNET_SEC_CERT_REV_FAILED) {
            assert(conn->security_flags & SECURITY_FLAG_IGNORE_REVOCATION);
            err = ERROR_SUCCESS;
        }
    }while(0);

    if(!err || conn->mask_errors) {
        CERT_CHAIN_POLICY_PARA policyPara;
        SSL_EXTRA_CERT_CHAIN_POLICY_PARA sslExtraPolicyPara;
        CERT_CHAIN_POLICY_STATUS policyStatus;
        CERT_CHAIN_CONTEXT chainCopy;

        /* Clear chain->TrustStatus.dwErrorStatus so
         * CertVerifyCertificateChainPolicy will verify additional checks
         * rather than stopping with an existing, ignored error.
         */
        memcpy(&chainCopy, chain, sizeof(chainCopy));
        chainCopy.TrustStatus.dwErrorStatus = 0;
        sslExtraPolicyPara.u.cbSize = sizeof(sslExtraPolicyPara);
        sslExtraPolicyPara.dwAuthType = AUTHTYPE_SERVER;
        sslExtraPolicyPara.pwszServerName = conn->server->name;
        sslExtraPolicyPara.fdwChecks = conn->security_flags;
        policyPara.cbSize = sizeof(policyPara);
        policyPara.dwFlags = 0;
        policyPara.pvExtraPolicyPara = &sslExtraPolicyPara;
        ret = CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL,
                &chainCopy, &policyPara, &policyStatus);
        /* Any error in the policy status indicates that the
         * policy couldn't be verified.
         */
        if(ret) {
            if(policyStatus.dwError == CERT_E_CN_NO_MATCH) {
                WARN("CERT_E_CN_NO_MATCH\n");
                if(conn->mask_errors)
                    conn->security_flags |= _SECURITY_FLAG_CERT_INVALID_CN;
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_SEC_CERT_CN_INVALID;
            }else if(policyStatus.dwError) {
                WARN("policyStatus.dwError %x\n", policyStatus.dwError);
                if(conn->mask_errors)
                    WARN("unknown error flags for policy status %x\n", policyStatus.dwError);
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_SEC_INVALID_CERT;
            }
        }else {
            err = GetLastError();
        }
    }

    if(err) {
        WARN("failed %u\n", err);
        CertFreeCertificateChain(chain);
        if(conn->server->cert_chain) {
            CertFreeCertificateChain(conn->server->cert_chain);
            conn->server->cert_chain = NULL;
        }
        if(conn->mask_errors)
            conn->server->security_flags |= conn->security_flags & _SECURITY_ERROR_FLAGS_MASK;
        return err;
    }

    /* FIXME: Reuse cached chain */
    if(conn->server->cert_chain)
        CertFreeCertificateChain(chain);
    else
        conn->server->cert_chain = chain;
    return ERROR_SUCCESS;
}

static int netconn_secure_verify(int preverify_ok, X509_STORE_CTX *ctx)
{
    SSL *ssl;
    BOOL ret = FALSE;
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
        CERT_STORE_CREATE_NEW_FLAG, NULL);
    netconn_t *conn;

    ssl = pX509_STORE_CTX_get_ex_data(ctx,
        pSSL_get_ex_data_X509_STORE_CTX_idx());
    conn = pSSL_get_ex_data(ssl, conn_idx);
    if (store)
    {
        X509 *cert;
        int i;
        PCCERT_CONTEXT endCert = NULL;
        struct stack_st *chain = (struct stack_st *)pX509_STORE_CTX_get_chain( ctx );

        ret = TRUE;
        for (i = 0; ret && i < psk_num(chain); i++)
        {
            PCCERT_CONTEXT context;

            cert = (X509 *)psk_value(chain, i);
            if ((context = X509_to_cert_context(cert)))
            {
                ret = CertAddCertificateContextToStore(store, context,
                        CERT_STORE_ADD_ALWAYS, i ? NULL : &endCert);
                CertFreeCertificateContext(context);
            }
        }
        if (!endCert) ret = FALSE;
        if (ret)
        {
            DWORD_PTR err = netconn_verify_cert(conn, endCert, store);

            if (err)
            {
                pSSL_set_ex_data(ssl, error_idx, (void *)err);
                ret = FALSE;
            }
        }
        CertFreeCertificateContext(endCert);
        CertCloseStore(store, 0);
    }
    return ret;
}

static long get_tls_option(void) {
    long tls_option = SSL_OP_NO_SSLv2; /* disable SSLv2 for security reason, secur32/Schannel(GnuTLS) don't support it */
#ifdef SSL_OP_NO_TLSv1_2
    DWORD type, val, size;
    HKEY hkey,tls12_client,tls11_client;
    LONG res;
    const WCHAR Schannel_Prot[] = {  /* SYSTEM\\CurrentControlSet\\Control\\SecurityProviders\\SCANNEL\\Protocols */
              'S','Y','S','T','E','M','\\',
              'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
              'C','o','n','t','r','o','l','\\',
              'S','e','c','u','r','i','t','y','P','r','o','v','i','d','e','r','s','\\',
              'S','C','H','A','N','N','E','L','\\',
              'P','r','o','t','o','c','o','l','s',0 };
     const WCHAR TLS12_Client[] = {'T','L','S',' ','1','.','2','\\','C','l','i','e','n','t',0};
     const WCHAR TLS11_Client[] = {'T','L','S',' ','1','.','1','\\','C','l','i','e','n','t',0};
     const WCHAR DisabledByDefault[] = {'D','i','s','a','b','l','e','d','B','y','D','e','f','a','u','l','t',0};

    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
          Schannel_Prot,
          0, KEY_READ, &hkey);
    if (res != ERROR_SUCCESS) { /* enabled TLSv1.1/1.2 when no registry entry */
        return tls_option;
    }
    if (RegOpenKeyExW(hkey, TLS12_Client, 0, KEY_READ, &tls12_client) == ERROR_SUCCESS) {
        size = sizeof(DWORD);
        if (RegQueryValueExW(tls12_client, DisabledByDefault, NULL, &type,  (LPBYTE) &val, &size) == ERROR_SUCCESS
            && type == REG_DWORD) {
            tls_option |= val?SSL_OP_NO_TLSv1_2:0;
        }
        RegCloseKey(tls12_client);
    }
    if (RegOpenKeyExW(hkey, TLS11_Client, 0, KEY_READ, &tls11_client) == ERROR_SUCCESS) {
        size = sizeof(DWORD);
        if (RegQueryValueExW(tls11_client, DisabledByDefault, NULL, &type,  (LPBYTE) &val, &size) == ERROR_SUCCESS
            && type == REG_DWORD) {
            tls_option |= val?SSL_OP_NO_TLSv1_1:0;
        }
        RegCloseKey(tls11_client);
    }
    RegCloseKey(hkey);
#endif
    return tls_option;
}

static CRITICAL_SECTION init_ssl_cs;
static CRITICAL_SECTION_DEBUG init_ssl_cs_debug =
{
    0, 0, &init_ssl_cs,
    { &init_ssl_cs_debug.ProcessLocksList,
      &init_ssl_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": init_ssl_cs") }
};
static CRITICAL_SECTION init_ssl_cs = { &init_ssl_cs_debug, -1, 0, 0, 0, 0 };

static DWORD init_openssl(void)
{
#ifdef SONAME_LIBCRYPTO
    unsigned int i;

    if(OpenSSL_ssl_handle)
        return ERROR_SUCCESS;

    OpenSSL_ssl_handle = wine_dlopen(SONAME_LIBSSL, RTLD_NOW, NULL, 0);
    if(!OpenSSL_ssl_handle) {
        ERR("trying to use a SSL connection, but couldn't load %s. Expect trouble.\n", SONAME_LIBSSL);
        return ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
    }

    OpenSSL_crypto_handle = wine_dlopen(SONAME_LIBCRYPTO, RTLD_NOW, NULL, 0);
    if(!OpenSSL_crypto_handle) {
        ERR("trying to use a SSL connection, but couldn't load %s. Expect trouble.\n", SONAME_LIBCRYPTO);
        return ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
    }

    /* mmm nice ugly macroness */
#define DYNSSL(x) \
    p##x = wine_dlsym(OpenSSL_ssl_handle, #x, NULL, 0); \
    if (!p##x) { \
        ERR("failed to load symbol %s\n", #x); \
        return ERROR_INTERNET_SECURITY_CHANNEL_ERROR; \
    }

    DYNSSL(SSL_library_init);
    DYNSSL(SSL_load_error_strings);
    DYNSSL(SSLv23_method);
    DYNSSL(SSL_CTX_free);
    DYNSSL(SSL_CTX_new);
    DYNSSL(SSL_CTX_ctrl);
    DYNSSL(SSL_new);
    DYNSSL(SSL_free);
    DYNSSL(SSL_ctrl);
    DYNSSL(SSL_set_fd);
    DYNSSL(SSL_connect);
    DYNSSL(SSL_shutdown);
    DYNSSL(SSL_write);
    DYNSSL(SSL_read);
    DYNSSL(SSL_pending);
    DYNSSL(SSL_get_error);
    DYNSSL(SSL_get_ex_new_index);
    DYNSSL(SSL_get_ex_data);
    DYNSSL(SSL_set_ex_data);
    DYNSSL(SSL_get_ex_data_X509_STORE_CTX_idx);
    DYNSSL(SSL_get_peer_certificate);
    DYNSSL(SSL_CTX_get_timeout);
    DYNSSL(SSL_CTX_set_timeout);
    DYNSSL(SSL_CTX_set_default_verify_paths);
    DYNSSL(SSL_CTX_set_verify);
    DYNSSL(SSL_get_current_cipher);
    DYNSSL(SSL_CIPHER_get_bits);
#undef DYNSSL

#define DYNCRYPTO(x) \
    p##x = wine_dlsym(OpenSSL_crypto_handle, #x, NULL, 0); \
    if (!p##x) { \
        ERR("failed to load symbol %s\n", #x); \
        return ERROR_INTERNET_SECURITY_CHANNEL_ERROR; \
    }

    DYNCRYPTO(BIO_new_fp);
    DYNCRYPTO(CRYPTO_num_locks);
    DYNCRYPTO(CRYPTO_set_id_callback);
    DYNCRYPTO(CRYPTO_set_locking_callback);
    DYNCRYPTO(ERR_free_strings);
    DYNCRYPTO(ERR_get_error);
    DYNCRYPTO(ERR_error_string);
    DYNCRYPTO(X509_STORE_CTX_get_ex_data);
    DYNCRYPTO(X509_STORE_CTX_get_chain);
    DYNCRYPTO(i2d_X509);
    DYNCRYPTO(sk_num);
    DYNCRYPTO(sk_value);
#undef DYNCRYPTO

#define pSSL_CTX_set_options(ctx,op) \
       pSSL_CTX_ctrl((ctx),SSL_CTRL_OPTIONS,(op),NULL)
#define pSSL_set_options(ssl,op) \
       pSSL_ctrl((ssl),SSL_CTRL_OPTIONS,(op),NULL)

    pSSL_library_init();
    pSSL_load_error_strings();
    pBIO_new_fp(stderr, BIO_NOCLOSE); /* FIXME: should use winedebug stuff */

    meth = pSSLv23_method();
    ctx = pSSL_CTX_new(meth);
    pSSL_CTX_set_options(ctx, get_tls_option());
    if(!pSSL_CTX_set_default_verify_paths(ctx)) {
        ERR("SSL_CTX_set_default_verify_paths failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        return ERROR_OUTOFMEMORY;
    }

    error_idx = pSSL_get_ex_new_index(0, (void *)"error index", NULL, NULL, NULL);
    if(error_idx == -1) {
        ERR("SSL_get_ex_new_index failed; %s\n", pERR_error_string(pERR_get_error(), 0));
        return ERROR_OUTOFMEMORY;
    }

    conn_idx = pSSL_get_ex_new_index(0, (void *)"netconn index", NULL, NULL, NULL);
    if(conn_idx == -1) {
        ERR("SSL_get_ex_new_index failed; %s\n", pERR_error_string(pERR_get_error(), 0));
        return ERROR_OUTOFMEMORY;
    }

    pSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, netconn_secure_verify);

    pCRYPTO_set_id_callback(ssl_thread_id);
    num_ssl_locks = pCRYPTO_num_locks();
    ssl_locks = heap_alloc(num_ssl_locks * sizeof(CRITICAL_SECTION));
    if(!ssl_locks)
        return ERROR_OUTOFMEMORY;

    for(i = 0; i < num_ssl_locks; i++)
    {
        InitializeCriticalSection(&ssl_locks[i]);
        ssl_locks[i].DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ssl_locks");
    }
    pCRYPTO_set_locking_callback(ssl_lock_callback);

    return ERROR_SUCCESS;
#else
    FIXME("can't use SSL, libcrypto not compiled in.\n");
    return ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
#endif
}
#endif /* SONAME_LIBSSL */

static DWORD create_netconn_socket(server_t *server, netconn_t *netconn, DWORD timeout)
{
    int result;
    ULONG flag;

    assert(server->addr_len);
    result = netconn->socket = socket(server->addr.ss_family, SOCK_STREAM, 0);
    if(result != -1) {
        flag = 1;
        ioctlsocket(netconn->socket, FIONBIO, &flag);
        result = connect(netconn->socket, (struct sockaddr*)&server->addr, server->addr_len);
        if(result == -1)
        {
            if (sock_get_error(errno) == WSAEINPROGRESS) {
                // ReactOS: use select instead of poll
                fd_set outfd;
                struct timeval tv;
                int res;

                FD_ZERO(&outfd);
                FD_SET(netconn->socket, &outfd);
                tv.tv_sec = timeout / 1000;
                tv.tv_usec = (timeout % 1000) * 1000;
                res = select(0, NULL, &outfd, NULL, &tv);
                if (!res)
                {
                    closesocket(netconn->socket);
                    return ERROR_INTERNET_CANNOT_CONNECT;
                }
                else if (res > 0)
                {
                    int err;
                    socklen_t len = sizeof(err);
                    if (!getsockopt(netconn->socket, SOL_SOCKET, SO_ERROR, (void *)&err, &len) && !err)
                        result = 0;
                }
            }
        }
        if(result == -1)
            closesocket(netconn->socket);
        else {
            flag = 0;
            ioctlsocket(netconn->socket, FIONBIO, &flag);
        }
    }
    if(result == -1)
        return ERROR_INTERNET_CANNOT_CONNECT;

#ifdef TCP_NODELAY
    flag = 1;
    result = setsockopt(netconn->socket, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag));
    if(result < 0)
        WARN("setsockopt(TCP_NODELAY) failed\n");
#endif

    return ERROR_SUCCESS;
}

DWORD create_netconn(BOOL useSSL, server_t *server, DWORD security_flags, BOOL mask_errors, DWORD timeout, netconn_t **ret)
{
    netconn_t *netconn;
    int result;

#ifdef SONAME_LIBSSL
    if(useSSL) {
        DWORD res;

        TRACE("using SSL connection\n");

        EnterCriticalSection(&init_ssl_cs);
        res = init_openssl();
        LeaveCriticalSection(&init_ssl_cs);
        if(res != ERROR_SUCCESS)
            return res;
    }
#endif

    netconn = heap_alloc_zero(sizeof(*netconn));
    if(!netconn)
        return ERROR_OUTOFMEMORY;

    netconn->socket = -1;
    netconn->security_flags = security_flags | server->security_flags;
    netconn->mask_errors = mask_errors;
    list_init(&netconn->pool_entry);

    result = create_netconn_socket(server, netconn, timeout);
    if (result != ERROR_SUCCESS) {
        heap_free(netconn);
        return result;
    }

    server_addref(server);
    netconn->server = server;
    *ret = netconn;
    return result;
}

void free_netconn(netconn_t *netconn)
{
    server_release(netconn->server);

    if (netconn->secure) {
#ifdef SONAME_LIBSSL
        pSSL_shutdown(netconn->ssl_s);
        pSSL_free(netconn->ssl_s);
#endif
    }

    closesocket(netconn->socket);
    heap_free(netconn);
}

void NETCON_unload(void)
{
#if defined(SONAME_LIBSSL) && defined(SONAME_LIBCRYPTO)
    if (OpenSSL_crypto_handle)
    {
        pERR_free_strings();
        wine_dlclose(OpenSSL_crypto_handle, NULL, 0);
    }
    if (OpenSSL_ssl_handle)
    {
        if (ctx)
            pSSL_CTX_free(ctx);
        wine_dlclose(OpenSSL_ssl_handle, NULL, 0);
    }
    if (ssl_locks)
    {
        unsigned int i;
        for (i = 0; i < num_ssl_locks; i++)
        {
            ssl_locks[i].DebugInfo->Spare[0] = 0;
            DeleteCriticalSection(&ssl_locks[i]);
        }
        heap_free(ssl_locks);
    }
#endif
}

#ifndef __REACTOS__
/* translate a unix error code into a winsock one */
int sock_get_error( int err )
{
#if !defined(__MINGW32__) && !defined (_MSC_VER)
    switch (err)
    {
        case EINTR:             return WSAEINTR;
        case EBADF:             return WSAEBADF;
        case EPERM:
        case EACCES:            return WSAEACCES;
        case EFAULT:            return WSAEFAULT;
        case EINVAL:            return WSAEINVAL;
        case EMFILE:            return WSAEMFILE;
        case EWOULDBLOCK:       return WSAEWOULDBLOCK;
        case EINPROGRESS:       return WSAEINPROGRESS;
        case EALREADY:          return WSAEALREADY;
        case ENOTSOCK:          return WSAENOTSOCK;
        case EDESTADDRREQ:      return WSAEDESTADDRREQ;
        case EMSGSIZE:          return WSAEMSGSIZE;
        case EPROTOTYPE:        return WSAEPROTOTYPE;
        case ENOPROTOOPT:       return WSAENOPROTOOPT;
        case EPROTONOSUPPORT:   return WSAEPROTONOSUPPORT;
        case ESOCKTNOSUPPORT:   return WSAESOCKTNOSUPPORT;
        case EOPNOTSUPP:        return WSAEOPNOTSUPP;
        case EPFNOSUPPORT:      return WSAEPFNOSUPPORT;
        case EAFNOSUPPORT:      return WSAEAFNOSUPPORT;
        case EADDRINUSE:        return WSAEADDRINUSE;
        case EADDRNOTAVAIL:     return WSAEADDRNOTAVAIL;
        case ENETDOWN:          return WSAENETDOWN;
        case ENETUNREACH:       return WSAENETUNREACH;
        case ENETRESET:         return WSAENETRESET;
        case ECONNABORTED:      return WSAECONNABORTED;
        case EPIPE:
        case ECONNRESET:        return WSAECONNRESET;
        case ENOBUFS:           return WSAENOBUFS;
        case EISCONN:           return WSAEISCONN;
        case ENOTCONN:          return WSAENOTCONN;
        case ESHUTDOWN:         return WSAESHUTDOWN;
        case ETOOMANYREFS:      return WSAETOOMANYREFS;
        case ETIMEDOUT:         return WSAETIMEDOUT;
        case ECONNREFUSED:      return WSAECONNREFUSED;
        case ELOOP:             return WSAELOOP;
        case ENAMETOOLONG:      return WSAENAMETOOLONG;
        case EHOSTDOWN:         return WSAEHOSTDOWN;
        case EHOSTUNREACH:      return WSAEHOSTUNREACH;
        case ENOTEMPTY:         return WSAENOTEMPTY;
#ifdef EPROCLIM
        case EPROCLIM:          return WSAEPROCLIM;
#endif
#ifdef EUSERS
        case EUSERS:            return WSAEUSERS;
#endif
#ifdef EDQUOT
        case EDQUOT:            return WSAEDQUOT;
#endif
#ifdef ESTALE
        case ESTALE:            return WSAESTALE;
#endif
#ifdef EREMOTE
        case EREMOTE:           return WSAEREMOTE;
#endif
    default: errno=err; perror("sock_set_error"); return WSAEFAULT;
    }
#endif
    return err;
}
#endif

#ifdef SONAME_LIBSSL
static DWORD netcon_secure_connect_setup(netconn_t *connection, long tls_option)
{
    void *ssl_s;
    DWORD res;
    int bits;

    ssl_s = pSSL_new(ctx);
    if (!ssl_s)
    {
        ERR("SSL_new failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        return ERROR_OUTOFMEMORY;
    }

    pSSL_set_options(ssl_s, tls_option);
    if (!pSSL_set_fd(ssl_s, connection->socket))
    {
        ERR("SSL_set_fd failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        res = ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
        goto fail;
    }

    if (!pSSL_set_ex_data(ssl_s, conn_idx, connection))
    {
        ERR("SSL_set_ex_data failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        res = ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
        goto fail;
    }
    if (pSSL_connect(ssl_s) <= 0)
    {
        res = (DWORD_PTR)pSSL_get_ex_data(ssl_s, error_idx);
        if (!res)
            res = ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
        ERR("SSL_connect failed: %d\n", res);
        goto fail;
    }

    connection->ssl_s = ssl_s;
    connection->secure = TRUE;

    bits = NETCON_GetCipherStrength(connection);
    if (bits >= 128)
        connection->security_flags |= SECURITY_FLAG_STRENGTH_STRONG;
    else if (bits >= 56)
        connection->security_flags |= SECURITY_FLAG_STRENGTH_MEDIUM;
    else
        connection->security_flags |= SECURITY_FLAG_STRENGTH_WEAK;
    connection->security_flags |= SECURITY_FLAG_SECURE;

    if(connection->mask_errors)
        connection->server->security_flags = connection->security_flags;
    return ERROR_SUCCESS;

fail:
    if (ssl_s)
    {
        pSSL_shutdown(ssl_s);
        pSSL_free(ssl_s);
    }
    return res;
}
#endif

/******************************************************************************
 * NETCON_secure_connect
 * Initiates a secure connection over an existing plaintext connection.
 */
DWORD NETCON_secure_connect(netconn_t *connection, server_t *server)
{
    DWORD res = ERROR_NOT_SUPPORTED;

    /* can't connect if we are already connected */
    if(connection->secure) {
        ERR("already connected\n");
        return ERROR_INTERNET_CANNOT_CONNECT;
    }

    if(server != connection->server) {
        server_release(connection->server);
        server_addref(server);
        connection->server = server;
    }

#ifdef SONAME_LIBSSL
    /* connect with given TLS options */
    res = netcon_secure_connect_setup(connection, get_tls_option());
    if (res == ERROR_SUCCESS)
        return res;

#ifdef SSL_OP_NO_TLSv1_2
    /* FIXME: when got version alert and FIN from server */
    /* fallback to connect without TLSv1.1/TLSv1.2        */
    if (res == ERROR_INTERNET_SECURITY_CHANNEL_ERROR)
    {
        closesocket(connection->socket);
        pSSL_CTX_set_options(ctx,SSL_OP_NO_TLSv1_1|SSL_OP_NO_TLSv1_2);
        res = create_netconn_socket(connection->server, connection, 500);
        if (res != ERROR_SUCCESS)
            return res;
        res = netcon_secure_connect_setup(connection, get_tls_option()|SSL_OP_NO_TLSv1_1|SSL_OP_NO_TLSv1_2);
    }
#endif
#else
    FIXME("Cannot connect, OpenSSL not available.\n");
#endif
    return res;
}

/******************************************************************************
 * NETCON_send
 * Basically calls 'send()' unless we should use SSL
 * number of chars send is put in *sent
 */
DWORD NETCON_send(netconn_t *connection, const void *msg, size_t len, int flags,
		int *sent /* out */)
{
    if(!connection->secure)
    {
	*sent = send(connection->socket, msg, len, flags);
	if (*sent == -1)
	    return sock_get_error(errno);
        return ERROR_SUCCESS;
    }
    else
    {
#ifdef SONAME_LIBSSL
        if(!connection->secure) {
            FIXME("not connected\n");
            return ERROR_NOT_SUPPORTED;
        }
	if (flags)
            FIXME("SSL_write doesn't support any flags (%08x)\n", flags);
	*sent = pSSL_write(connection->ssl_s, msg, len);
	if (*sent < 1 && len)
	    return ERROR_INTERNET_CONNECTION_ABORTED;
        return ERROR_SUCCESS;
#else
        FIXME("not supported on this platform\n");
	return ERROR_NOT_SUPPORTED;
#endif
    }
}

/******************************************************************************
 * NETCON_recv
 * Basically calls 'recv()' unless we should use SSL
 * number of chars received is put in *recvd
 */
DWORD NETCON_recv(netconn_t *connection, void *buf, size_t len, int flags, int *recvd)
{
    *recvd = 0;
    if (!len)
        return ERROR_SUCCESS;

    if (!connection->secure)
    {
	*recvd = recv(connection->socket, buf, len, flags);
	return *recvd == -1 ? sock_get_error(errno) :  ERROR_SUCCESS;
    }
    else
    {
#ifdef SONAME_LIBSSL
        if(!connection->secure) {
            FIXME("not connected\n");
            return ERROR_NOT_SUPPORTED;
        }
	*recvd = pSSL_read(connection->ssl_s, buf, len);

        /* Check if EOF was received */
        if(!*recvd && (pSSL_get_error(connection->ssl_s, *recvd)==SSL_ERROR_ZERO_RETURN
                    || pSSL_get_error(connection->ssl_s, *recvd)==SSL_ERROR_SYSCALL))
            return ERROR_SUCCESS;

        return *recvd > 0 ? ERROR_SUCCESS : ERROR_INTERNET_CONNECTION_ABORTED;
#else
        FIXME("not supported on this platform\n");
	return ERROR_NOT_SUPPORTED;
#endif
    }
}

/******************************************************************************
 * NETCON_query_data_available
 * Returns the number of bytes of peeked data plus the number of bytes of
 * queued, but unread data.
 */
BOOL NETCON_query_data_available(netconn_t *connection, DWORD *available)
{
    *available = 0;

    if(!connection->secure)
    {
#ifdef FIONREAD
        ULONG unread;
        int retval = ioctlsocket(connection->socket, FIONREAD, &unread);
        if (!retval)
        {
            TRACE("%d bytes of queued, but unread data\n", unread);
            *available += unread;
        }
#endif
    }
    else
    {
#ifdef SONAME_LIBSSL
        *available = pSSL_pending(connection->ssl_s);
#else
        FIXME("not supported on this platform\n");
        return FALSE;
#endif
    }
    return TRUE;
}

BOOL NETCON_is_alive(netconn_t *netconn)
{
#ifdef MSG_DONTWAIT
    ssize_t len;
    BYTE b;

    len = recv(netconn->socket, &b, 1, MSG_PEEK|MSG_DONTWAIT);
    return len == 1 || (len == -1 && errno == EWOULDBLOCK);
#elif defined(__MINGW32__) || defined(_MSC_VER)
    ULONG mode;
    int len;
    char b;

    mode = 1;
    if(!ioctlsocket(netconn->socket, FIONBIO, &mode))
        return FALSE;

    len = recv(netconn->socket, &b, 1, MSG_PEEK);

    mode = 0;
    if(!ioctlsocket(netconn->socket, FIONBIO, &mode))
        return FALSE;

    return len == 1 || (len == -1 && errno == WSAEWOULDBLOCK);
#else
    FIXME("not supported on this platform\n");
    return TRUE;
#endif
}

LPCVOID NETCON_GetCert(netconn_t *connection)
{
#ifdef SONAME_LIBSSL
    X509* cert;
    LPCVOID r = NULL;

    if (!connection->secure)
        return NULL;

    cert = pSSL_get_peer_certificate(connection->ssl_s);
    r = X509_to_cert_context(cert);
    return r;
#else
    FIXME("not supported on this platform\n");
    return NULL;
#endif
}

int NETCON_GetCipherStrength(netconn_t *connection)
{
#ifdef SONAME_LIBSSL
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x0090707f)
    const SSL_CIPHER *cipher;
#else
    SSL_CIPHER *cipher;
#endif
    int bits = 0;

    if (!connection->secure)
        return 0;
    cipher = pSSL_get_current_cipher(connection->ssl_s);
    if (!cipher)
        return 0;
    pSSL_CIPHER_get_bits(cipher, &bits);
    return bits;
#else
    FIXME("not supported on this platform\n");
    return 0;
#endif
}

DWORD NETCON_set_timeout(netconn_t *connection, BOOL send, DWORD value)
{
    int result;
    struct timeval tv;

    /* value is in milliseconds, convert to struct timeval */
    if (value == INFINITE)
    {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    }
    else
    {
        tv.tv_sec = value / 1000;
        tv.tv_usec = (value % 1000) * 1000;
    }
    result = setsockopt(connection->socket, SOL_SOCKET,
                        send ? SO_SNDTIMEO : SO_RCVTIMEO, (void*)&tv,
                        sizeof(tv));
    if (result == -1)
    {
        WARN("setsockopt failed (%s)\n", strerror(errno));
        return sock_get_error(errno);
    }
    return ERROR_SUCCESS;
}
