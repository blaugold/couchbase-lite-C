#include "emscripten/bind.h"
#include "emscripten/threading.h"
#include "fleece/Fleece.h"
#include "fleece/Fleece.hh"
#include "fleece/slice.hh"
#include "cbl/CouchbaseLite.h"
#include <mutex>

using namespace emscripten;
using namespace fleece;

static uintptr_t _CBL_malloc(size_t size) {
    return (uintptr_t)malloc(size);
}

static void _CBL_free(uintptr_t ptr) {
    free((void*)ptr);
}

EMSCRIPTEN_BINDINGS(CBLMemory) {
    function("CBL_malloc", &_CBL_malloc);
    function("CBL_free", &_CBL_free);
}

// === Fleece ======================================================================================

static uintptr_t _FLSlice_GetBuf(const FLSlice& self) {
    return (uintptr_t)self.buf;
}

static void _FLSlice_SetBuf(FLSlice& self, uintptr_t buf) {
    self.buf = (const void *)buf;
}

static FLSlice _FLSlice_FromPointer(uintptr_t slicePointerInt, size_t index) {
    auto slicePointer = (FLSlice *)slicePointerInt;
    return slicePointer[index];
}

static uintptr_t _FLSliceResult_GetBuf(const FLSliceResult& self) {
    return (uintptr_t)self.buf;
}

static void _FLSliceResult_SetBuf(FLSliceResult& self, uintptr_t buf) {
    self.buf = (const void *)buf;
}

static void _FLSliceResult_Release(FLSliceResult& self) {
    FLSliceResult_Release(self);
}

EMSCRIPTEN_BINDINGS(FLSlice) {
    class_<FLSlice>("FLSlice")
        .constructor()
        .property("buf", &_FLSlice_GetBuf, &_FLSlice_SetBuf)
        .property("size", &FLSlice::size);

    function("FLSlice_FromPointer", &_FLSlice_FromPointer);

    class_<FLSliceResult>("FLSliceResult")
        .constructor()
        .property("buf", &_FLSliceResult_GetBuf, &_FLSliceResult_SetBuf)
        .property("size", &FLSliceResult::size);

    function("FLSliceResult_Release", &_FLSliceResult_Release);
}

// === Couchbase Lite ==============================================================================

// --- CBL_Edition.h -------------------------------------------------------------------------------

EMSCRIPTEN_BINDINGS(CBL_Edition) {
    constant<std::string>("CBLITE_VERSION", CBLITE_VERSION);
    constant<uint32_t>("CBLITE_VERSION_NUMBER", CBLITE_VERSION_NUMBER);
    constant<uint32_t>("CBLITE_BUILD_NUMBER", CBLITE_BUILD_NUMBER);
    constant<std::string>("CBLITE_SOURCE_ID", CBLITE_VERSION);
}

// --- CBLBase.h -----------------------------------------------------------------------------------

static uintptr_t _CBLRefCounted_Retain(uintptr_t self) {
    return (uintptr_t)CBL_Retain((CBLRefCounted *)self);
}
static void _CBLRefCounted_Release(uintptr_t self) {
    CBL_Release((CBLRefCounted *)self);
}

static void _CBLListener_Remove(uintptr_t self) {
    CBLListener_Remove((CBLListenerToken *)self);
}

EMSCRIPTEN_BINDINGS(CBLBase) {
    enum_<CBLErrorDomain>("CBLErrorDomain")
        .value("CBL", kCBLDomain)
        .value("POSIX", kCBLPOSIXDomain)
        .value("SQLite", kCBLSQLiteDomain)
        .value("Fleece", kCBLFleeceDomain)
        .value("Network", kCBLNetworkDomain)
        .value("WebSocket", kCBLWebSocketDomain);

    enum_<CBLErrorCode>("CBLErrorCode")
        .value("AssertionFailed", kCBLErrorAssertionFailed)
        .value("Unimplemented", kCBLErrorUnimplemented)
        .value("UnsupportedEncryption", kCBLErrorUnsupportedEncryption)
        .value("BadRevisionID", kCBLErrorBadRevisionID)
        .value("CorruptRevisionData", kCBLErrorCorruptRevisionData)
        .value("NotOpen", kCBLErrorNotOpen)
        .value("NotFound", kCBLErrorNotFound)
        .value("Conflict", kCBLErrorConflict)
        .value("InvalidParameter", kCBLErrorInvalidParameter)
        .value("UnexpectedError", kCBLErrorUnexpectedError)
        .value("CantOpenFile", kCBLErrorCantOpenFile)
        .value("IOError", kCBLErrorIOError)
        .value("MemoryError", kCBLErrorMemoryError)
        .value("NotWriteable", kCBLErrorNotWriteable)
        .value("CorruptData", kCBLErrorCorruptData)
        .value("Busy", kCBLErrorBusy)
        .value("NotInTransaction", kCBLErrorNotInTransaction)
        .value("TransactionNotClosed", kCBLErrorTransactionNotClosed)
        .value("Unsupported", kCBLErrorUnsupported)
        .value("NotADatabaseFile", kCBLErrorNotADatabaseFile)
        .value("WrongFormat", kCBLErrorWrongFormat)
        .value("Crypto", kCBLErrorCrypto)
        .value("InvalidQuery", kCBLErrorInvalidQuery)
        .value("MissingIndex", kCBLErrorMissingIndex)
        .value("InvalidQueryParam", kCBLErrorInvalidQueryParam)
        .value("RemoteError", kCBLErrorRemoteError)
        .value("DatabaseTooOld", kCBLErrorDatabaseTooOld)
        .value("DatabaseTooNew", kCBLErrorDatabaseTooNew)
        .value("BadDocID", kCBLErrorBadDocID)
        .value("CantUpgradeDatabase", kCBLErrorCantUpgradeDatabase);

    enum_<CBLNetworkErrorCode>("CBLNetworkErrorCode")
        .value("DNSFailure", kCBLNetErrDNSFailure)
        .value("UnknownHost", kCBLNetErrUnknownHost)
        .value("Timeout", kCBLNetErrTimeout)
        .value("InvalidURL", kCBLNetErrInvalidURL)
        .value("TooManyRedirects", kCBLNetErrTooManyRedirects)
        .value("TLSHandshakeFailed", kCBLNetErrTLSHandshakeFailed)
        .value("TLSCertExpired", kCBLNetErrTLSCertExpired)
        .value("TLSCertUntrusted", kCBLNetErrTLSCertUntrusted)
        .value("TLSClientCertRequired", kCBLNetErrTLSClientCertRequired)
        .value("TLSClientCertRejected", kCBLNetErrTLSClientCertRejected)
        .value("TLSCertUnknownRoot", kCBLNetErrTLSCertUnknownRoot)
        .value("InvalidRedirect", kCBLNetErrInvalidRedirect)
        .value("Unknown", kCBLNetErrUnknown)
        .value("TLSCertRevoked", kCBLNetErrTLSCertRevoked)
        .value("TLSCertNameMismatch", kCBLNetErrTLSCertNameMismatch);

    class_<CBLError>("CBLError")
        .constructor()
        .property("domain", &CBLError::domain)
        .property("code", &CBLError::code);

    function("CBLError_Message", &CBLError_Message, allow_raw_pointers());

    function("CBL_Now", &CBL_Now);

    function("CBL_Retain", &_CBLRefCounted_Retain);
    function("CBL_Release", &_CBLRefCounted_Release);
    function("CBL_InstanceCount", &CBL_InstanceCount);
    function("CBL_DumpInstances", &CBL_DumpInstances);

    function("CBLListener_Remove", &_CBLListener_Remove);
}

// --- CBLLog.h ------------------------------------------------------------------------------------

std::mutex _CBLLog_Mutex;
uintptr_t _CBLLog_JSLogCallback = 0;

struct _CBLLog_LogMessage {
    _CBLLog_LogMessage() {}

    _CBLLog_LogMessage(CBLLogDomain domain, CBLLogLevel level, FLSlice message)
        : domain(domain)
        , level(level)
        , _message(message) {}

    static _CBLLog_LogMessage* FromPointer(uintptr_t ptr) {
        return reinterpret_cast<_CBLLog_LogMessage*>(ptr);
    }

    FLSlice message() const {
        return _message;
    }

    CBLLogDomain domain;
    CBLLogLevel level;
    alloc_slice _message;
};

static void _CBLLog_JSLogCallbackInvoker(CBLLogDomain domain, CBLLogLevel level, FLSlice message) {
    uintptr_t jsLogCallback = 0;
    {
        std::lock_guard<std::mutex> lock(_CBLLog_Mutex);
        jsLogCallback = _CBLLog_JSLogCallback;
    }
    if (!jsLogCallback)
        return;

    // The _CBLLog_LogMessage has to be allocated on the heap because it will be
    // passed to the JS callback asynchronously. The callback has to delete it.
    // Note that we might leak this message if the JS callback is removed while this message is
    // in the queue.
    auto logMessage = new _CBLLog_LogMessage(domain, level, message);

    // We call the JS callback asynchronously because we don't want to block the current thread
    // until the main thread is able to handle the log message.
    emscripten_async_run_in_main_runtime_thread(EM_FUNC_SIG_VI, jsLogCallback, logMessage);
}

static uintptr_t _CBLLog_Callback() {
    std::scoped_lock<std::mutex> lock(_CBLLog_Mutex);
    return _CBLLog_JSLogCallback;
}

static void _CBLLog_SetCallback(uintptr_t callback) {
    std::scoped_lock<std::mutex> lock(_CBLLog_Mutex);
    _CBLLog_JSLogCallback = callback;
    if (_CBLLog_JSLogCallback) {
        CBLLog_SetCallback(&_CBLLog_JSLogCallbackInvoker);
    } else {
        CBLLog_SetCallback(nullptr);
    }
}

EMSCRIPTEN_BINDINGS(CBLLog) {
    enum_<CBLLogDomain>("CBLLogDomain")
        .value("Database", kCBLLogDomainDatabase)
        .value("Query", kCBLLogDomainQuery)
        .value("Replicator", kCBLLogDomainReplicator)
        .value("Network", kCBLLogDomainNetwork);

    enum_<CBLLogLevel>("CBLLogLevel")
        .value("Debug", kCBLLogDebug)
        .value("Verbose", kCBLLogVerbose)
        .value("Info", kCBLLogInfo)
        .value("Warning", kCBLLogWarning)
        .value("Error", kCBLLogError)
        .value("None", kCBLLogNone);

    function("CBL_LogMessage", &CBL_LogMessage);

    function("CBLLog_ConsoleLevel", &CBLLog_ConsoleLevel);
    function("CBLLog_SetConsoleLevel", &CBLLog_SetConsoleLevel);

    class_<_CBLLog_LogMessage>("_CBLLog_LogMessage")
        .constructor()
        .property("domain", &_CBLLog_LogMessage::domain)
        .property("level", &_CBLLog_LogMessage::level)
        .property("message", &_CBLLog_LogMessage::message);

    function(
        "_CBLLog_LogMessage_FromPointer",
        &_CBLLog_LogMessage::FromPointer,
        allow_raw_pointers()
    );

    function("CBLLog_CallbackLevel", &CBLLog_CallbackLevel);
    function("CBLLog_SetCallbackLevel", &CBLLog_SetCallbackLevel);
    function("CBLLog_Callback", &_CBLLog_Callback);
    function("CBLLog_SetCallback", &_CBLLog_SetCallback);

    class_<CBLLogFileConfiguration>("CBLLogFileConfiguration")
        .constructor()
        .property("level", &CBLLogFileConfiguration::level)
        .property("directory", &CBLLogFileConfiguration::directory)
        .property("maxRotateCount", &CBLLogFileConfiguration::maxRotateCount)
        .property("maxSize", &CBLLogFileConfiguration::maxSize)
        .property("usePlaintext", &CBLLogFileConfiguration::usePlaintext);

    function("CBLLog_FileConfig", &CBLLog_FileConfig, allow_raw_pointers());
    function("CBLLog_SetFileConfig", &CBLLog_SetFileConfig, allow_raw_pointers());
}

// --- CBLDatabase.h -------------------------------------------------------------------------------

static void CBLDatabase_DispatchNotificationOnMainRuntimeThread(void* context, CBLDatabase* db) {
    emscripten_async_run_in_main_runtime_thread(
        EM_FUNC_SIG_VI,
        &CBLDatabase_SendNotifications,
        db
    );
}

static uintptr_t _CBLDatabase_Open(FLSlice name,
                        const CBLDatabaseConfiguration* config,
                        CBLError* outError) {
    auto db = CBLDatabase_Open(name, config, outError);

    if (db) {
        CBLDatabase_BufferNotifications(
            db,
            &CBLDatabase_DispatchNotificationOnMainRuntimeThread,
            nullptr
        );
    }

    return (uintptr_t)db;
}

static bool _CBLDatabase_Close(uintptr_t self, CBLError* outError) {
    return CBLDatabase_Close((CBLDatabase*)self, outError);
}

static bool _CBLDatabase_Delete(uintptr_t self, CBLError* outError) {
    return CBLDatabase_Delete((CBLDatabase*)self, outError);
}

static bool _CBLDatabase_BeginTransaction(uintptr_t self, CBLError* outError) {
    return CBLDatabase_BeginTransaction((CBLDatabase*)self, outError);
}

static bool _CBLDatabase_EndTransaction(uintptr_t self, bool commit, CBLError* outError) {
    return CBLDatabase_EndTransaction((CBLDatabase*)self, commit, outError);
}

#ifdef COUCHBASE_ENTERPRISE
static bool _CBLDatabase_ChangeEncryptionKey(uintptr_t self,
                                             const CBLEncryptionKey* newKey,
                                             CBLError* outError) {
    return CBLDatabase_ChangeEncryptionKey((CBLDatabase*)self, newKey, outError);
}
#endif

static bool _CBLDatabase_PerformMaintenance(uintptr_t self,
                                            CBLMaintenanceType type,
                                            CBLError* outError) {
    return CBLDatabase_PerformMaintenance((CBLDatabase*)self, type, outError);
}

static FLString _CBLDatabase_Name(uintptr_t self) {
    return CBLDatabase_Name((CBLDatabase*)self);
}

static FLStringResult _CBLDatabase_Path(uintptr_t self) {
    return CBLDatabase_Path((CBLDatabase*)self);
}

static uint64_t _CBLDatabase_Count(uintptr_t self) {
    return CBLDatabase_Count((CBLDatabase*)self);
}

static const CBLDatabaseConfiguration _CBLDatabase_Config(uintptr_t self) {
    return CBLDatabase_Config((CBLDatabase*)self);
}

static uintptr_t _CBLDatabase_AddChangeListener(uintptr_t self, uintptr_t listener) {
    return (uintptr_t)CBLDatabase_AddChangeListener((CBLDatabase*)self,
                                                    (CBLDatabaseChangeListener)listener,
                                                    nullptr);
}

EMSCRIPTEN_BINDINGS(CBLDatabase) {
#ifdef COUCHBASE_ENTERPRISE
    enum_<CBLEncryptionAlgorithm>("CBLEncryptionAlgorithm")
        .value("None", kCBLEncryptionNone)
        .value("AES256", kCBLEncryptionAES256);

    enum_<CBLEncryptionKeySize>("CBLEncryptionKeySize")
        .value("AES256", kCBLEncryptionKeySizeAES256);

    class_<CBLEncryptionKey>("CBLEncryptionKey")
        .constructor()
        .property("algorithm", &CBLEncryptionKey::algorithm)
        .property("bytes", &CBLEncryptionKey::bytes);

    value_array<std::array<uint8_t, 32>>("array_uint8_t_32")
        .element(index<0>())
        .element(index<1>())
        .element(index<2>())
        .element(index<3>())
        .element(index<4>())
        .element(index<5>())
        .element(index<6>())
        .element(index<7>())
        .element(index<8>())
        .element(index<9>())
        .element(index<10>())
        .element(index<11>())
        .element(index<12>())
        .element(index<13>())
        .element(index<14>())
        .element(index<15>())
        .element(index<16>())
        .element(index<17>())
        .element(index<18>())
        .element(index<19>())
        .element(index<20>())
        .element(index<21>())
        .element(index<22>())
        .element(index<23>())
        .element(index<24>())
        .element(index<25>())
        .element(index<26>())
        .element(index<27>())
        .element(index<28>())
        .element(index<29>())
        .element(index<30>())
        .element(index<31>());
#endif

    class_<CBLDatabaseConfiguration>("CBLDatabaseConfiguration")
        .constructor()
        .property("directory", &CBLDatabaseConfiguration::directory)
#ifdef COUCHBASE_ENTERPRISE
        .property("encryptionKey", &CBLDatabaseConfiguration::encryptionKey)
#endif
        ;

    function("CBLDatabaseConfiguration_Default", &CBLDatabaseConfiguration_Default);

#ifdef COUCHBASE_ENTERPRISE
    function("CBLEncryptionKey_FromPassword", &CBLEncryptionKey_FromPassword, allow_raw_pointers());
#endif

    function("CBL_DatabaseExists", &CBL_DatabaseExists);
    function("CBL_CopyDatabase", &CBL_CopyDatabase, allow_raw_pointers());
    function("CBL_DeleteDatabase", &CBL_DeleteDatabase, allow_raw_pointers());

    function("CBLDatabase_Open", &_CBLDatabase_Open, allow_raw_pointers());
    function("CBLDatabase_Close", &_CBLDatabase_Close, allow_raw_pointers());
    function("CBLDatabase_Delete", &_CBLDatabase_Delete, allow_raw_pointers());
    function("CBLDatabase_BeginTransaction", &_CBLDatabase_BeginTransaction, allow_raw_pointers());
    function("CBLDatabase_EndTransaction", &_CBLDatabase_EndTransaction, allow_raw_pointers());

#ifdef COUCHBASE_ENTERPRISE
    function(
        "CBLDatabase_ChangeEncryptionKey",
        &_CBLDatabase_ChangeEncryptionKey,
        allow_raw_pointers()
    );
#endif

    enum_<CBLMaintenanceType>("CBLMaintenanceType")
        .value("Compact", kCBLMaintenanceTypeCompact)
        .value("Reindex", kCBLMaintenanceTypeReindex)
        .value("IntegrityCheck", kCBLMaintenanceTypeIntegrityCheck)
        .value("Optimize", kCBLMaintenanceTypeOptimize)
        .value("FullOptimize", kCBLMaintenanceTypeFullOptimize);

    function(
        "CBLDatabase_PerformMaintenance",
        &_CBLDatabase_PerformMaintenance,
        allow_raw_pointers()
    );

    function("CBLDatabase_Name", &_CBLDatabase_Name, allow_raw_pointers());
    function("CBLDatabase_Path", &_CBLDatabase_Path, allow_raw_pointers());
    function("CBLDatabase_Count", &_CBLDatabase_Count);
    function("CBLDatabase_Config", &_CBLDatabase_Config);

    function(
        "CBLDatabase_AddChangeListener",
        &_CBLDatabase_AddChangeListener,
        allow_raw_pointers()
    );
}

// --- CBLDocument.h -------------------------------------------------------------------------------

static uintptr_t _CBLDatabase_GetDocument(uintptr_t db, FLString docID, CBLError* outError) {
    return (uintptr_t)CBLDatabase_GetDocument((CBLDatabase*)db, docID, outError);
}

static bool _CBLDatabase_SaveDocument(uintptr_t db, uintptr_t doc, CBLError* outError) {
    return CBLDatabase_SaveDocument((CBLDatabase*)db, (CBLDocument*)doc, outError);
}

static bool _CBLDatabase_SaveDocumentWithConcurrencyControl(uintptr_t db,
                                                            uintptr_t doc,
                                                            CBLConcurrencyControl concurrency,
                                                            CBLError* outError) {
    return CBLDatabase_SaveDocumentWithConcurrencyControl((CBLDatabase*)db,
                                                          (CBLDocument*)doc,
                                                          concurrency,
                                                          outError);
}

static bool _CBLDatabase_JSConflictHandlerInvoker(void* context,
                                                  CBLDocument* documentBeingSaved,
                                                  const CBLDocument* conflictingDocument) {
    return (bool)emscripten_sync_run_in_main_runtime_thread(
        EM_FUNC_SIG_III,
        context,
        documentBeingSaved,
        conflictingDocument
    );
}

static bool _CBLDatabase_SaveDocumentWithConflictHandler(uintptr_t db,
                                                        uintptr_t doc,
                                                        uintptr_t conflictHandler,
                                                        CBLError* outError){
    return CBLDatabase_SaveDocumentWithConflictHandler((CBLDatabase*)db,
                                                       (CBLDocument*)doc,
                                                       &_CBLDatabase_JSConflictHandlerInvoker,
                                                       (void*)conflictHandler,
                                                       outError);
}

static bool _CBLDatabase_DeleteDocument(uintptr_t db, uintptr_t document, CBLError* outError) {
    return CBLDatabase_DeleteDocument((CBLDatabase*)db, (CBLDocument*)document, outError);
}

static bool _CBLDatabase_DeleteDocumentWithConcurrencyControl(uintptr_t db,
                                                              uintptr_t document,
                                                              CBLConcurrencyControl concurrency,
                                                              CBLError* outError) {
    return CBLDatabase_DeleteDocumentWithConcurrencyControl((CBLDatabase*)db,
                                                            (CBLDocument*)document,
                                                            concurrency,
                                                            outError);
}

static bool _CBLDatabase_PurgeDocument(uintptr_t db, uintptr_t document, CBLError* outError) {
    return CBLDatabase_PurgeDocument((CBLDatabase*)db, (CBLDocument*)document, outError);
}

static bool _CBLDatabase_PurgeDocumentByID(uintptr_t database, FLString docID, CBLError* outError) {
    return CBLDatabase_PurgeDocumentByID((CBLDatabase*)database, docID, outError);
}

static uintptr_t _CBLDatabase_GetMutableDocument(uintptr_t database,
                                                 FLString docID,
                                                 CBLError* outError) {
    return (uintptr_t)CBLDatabase_GetMutableDocument((CBLDatabase*)database, docID, outError);
}

static uintptr_t _CBLDocument_Create(void) {
    return (uintptr_t)CBLDocument_Create();
}

static uintptr_t _CBLDocument_CreateWithID(FLString docID) {
    return (uintptr_t)CBLDocument_CreateWithID(docID);
}

static uintptr_t _CBLDocument_MutableCopy(uintptr_t original) {
    return (uintptr_t)CBLDocument_MutableCopy((CBLDocument*)original);
}

static FLString _CBLDocument_ID(uintptr_t doc) {
    return CBLDocument_ID((CBLDocument*)doc);
}

static FLString _CBLDocument_RevisionID(uintptr_t doc) {
    return CBLDocument_RevisionID((CBLDocument*)doc);
}

static uint64_t _CBLDocument_Sequence(uintptr_t doc) {
    return CBLDocument_Sequence((CBLDocument*)doc);
}

static uintptr_t _CBLDocument_Properties(uintptr_t doc) {
    return (uintptr_t)CBLDocument_Properties((CBLDocument*)doc);
}

static uintptr_t _CBLDocument_MutableProperties(uintptr_t doc) {
    return (uintptr_t)CBLDocument_MutableProperties((CBLDocument*)doc);
}

static void _CBLDocument_SetProperties(uintptr_t doc, uintptr_t properties) {
    CBLDocument_SetProperties((CBLDocument*)doc, (FLMutableDict)properties);
}

static FLSliceResult _CBLDocument_CreateJSON(uintptr_t doc) {
    return CBLDocument_CreateJSON((CBLDocument*)doc);
}

static bool _CBLDocument_SetJSON(uintptr_t doc, FLSlice json, CBLError* outError) {
    return CBLDocument_SetJSON((CBLDocument*)doc, json, outError);
}

static CBLTimestamp _CBLDatabase_GetDocumentExpiration(uintptr_t db,
                                                      FLSlice docID,
                                                      CBLError* outError) {
    return CBLDatabase_GetDocumentExpiration((CBLDatabase*)db, docID, outError);
}

static bool _CBLDatabase_SetDocumentExpiration(uintptr_t db,
                                              FLSlice docID,
                                              CBLTimestamp expiration,
                                              CBLError* outError) {
    return CBLDatabase_SetDocumentExpiration((CBLDatabase*)db, docID, expiration, outError);
}

static uintptr_t _CBLDatabase_AddDocumentChangeListener(uintptr_t db,
                                                        FLString docID,
                                                        uintptr_t listener) {
    return (uintptr_t)CBLDatabase_AddDocumentChangeListener((CBLDatabase*)db,
                                                            docID,
                                                            (CBLDocumentChangeListener)listener,
                                                            nullptr);
}

EMSCRIPTEN_BINDINGS(CBLDocument) {
    constant("kCBLTypeProperty", kCBLTypeProperty);

    enum_<CBLConcurrencyControl>("CBLConcurrencyControl")
        .value("LastWriteWins", kCBLConcurrencyControlLastWriteWins)
        .value("FailOnConflict", kCBLConcurrencyControlFailOnConflict);

    function("CBLDatabase_GetDocument", &_CBLDatabase_GetDocument, allow_raw_pointers());
    function("CBLDatabase_SaveDocument", &_CBLDatabase_SaveDocument, allow_raw_pointers());
    function(
        "CBLDatabase_SaveDocumentWithConcurrencyControl",
        &_CBLDatabase_SaveDocumentWithConcurrencyControl,
        allow_raw_pointers()
    );
    function(
        "CBLDatabase_SaveDocumentWithConflictHandler",
        &_CBLDatabase_SaveDocumentWithConflictHandler,
        allow_raw_pointers()
    );
    function("CBLDatabase_DeleteDocument", &_CBLDatabase_DeleteDocument, allow_raw_pointers());
    function(
        "CBLDatabase_DeleteDocumentWithConcurrencyControl",
        &_CBLDatabase_DeleteDocumentWithConcurrencyControl,
        allow_raw_pointers()
    );
    function("CBLDatabase_PurgeDocument", &_CBLDatabase_PurgeDocument, allow_raw_pointers());
    function(
        "CBLDatabase_PurgeDocumentByID",
        &_CBLDatabase_PurgeDocumentByID,
        allow_raw_pointers()
    );
    function(
        "CBLDatabase_GetMutableDocument",
        &_CBLDatabase_GetMutableDocument,
        allow_raw_pointers()
    );
    function("CBLDocument_Create", &_CBLDocument_Create);
    function("CBLDocument_CreateWithID", &_CBLDocument_CreateWithID);
    function("CBLDocument_MutableCopy", &_CBLDocument_MutableCopy);
    function("CBLDocument_ID", &_CBLDocument_ID);
    function("CBLDocument_RevisionID", &_CBLDocument_RevisionID);
    function("CBLDocument_Sequence", &_CBLDocument_Sequence);
    function("CBLDocument_Properties", &_CBLDocument_Properties);
    function("CBLDocument_MutableProperties", &_CBLDocument_MutableProperties);
    function("CBLDocument_SetProperties", &_CBLDocument_SetProperties);
    function("CBLDocument_CreateJSON", &_CBLDocument_CreateJSON);
    function("CBLDocument_SetJSON", &_CBLDocument_SetJSON, allow_raw_pointers());
    function(
        "CBLDatabase_GetDocumentExpiration",
        &_CBLDatabase_GetDocumentExpiration,
        allow_raw_pointers()
    );
    function(
        "CBLDatabase_SetDocumentExpiration",
        &_CBLDatabase_SetDocumentExpiration,
        allow_raw_pointers()
    );
    function(
        "CBLDatabase_AddDocumentChangeListener",
        &_CBLDatabase_AddDocumentChangeListener,
        allow_raw_pointers()
    );
}

// --- CBLBlob.h -----------------------------------------------------------------------------------

// TODO

// --- Encryptable.h -------------------------------------------------------------------------------

// TODO

// --- CBLQuery.h ----------------------------------------------------------------------------------

// TODO

// --- CBLReplicator.h -----------------------------------------------------------------------------

// TODO
