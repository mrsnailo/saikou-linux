#ifndef KONAN_LIBSAIKOU_CORE_H
#define KONAN_LIBSAIKOU_CORE_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
typedef bool            libsaikou_core_KBoolean;
#else
typedef _Bool           libsaikou_core_KBoolean;
#endif
typedef unsigned short     libsaikou_core_KChar;
typedef signed char        libsaikou_core_KByte;
typedef short              libsaikou_core_KShort;
typedef int                libsaikou_core_KInt;
typedef long long          libsaikou_core_KLong;
typedef unsigned char      libsaikou_core_KUByte;
typedef unsigned short     libsaikou_core_KUShort;
typedef unsigned int       libsaikou_core_KUInt;
typedef unsigned long long libsaikou_core_KULong;
typedef float              libsaikou_core_KFloat;
typedef double             libsaikou_core_KDouble;
typedef float __attribute__ ((__vector_size__ (16))) libsaikou_core_KVector128;
typedef void*              libsaikou_core_KNativePtr;
struct libsaikou_core_KType;
typedef struct libsaikou_core_KType libsaikou_core_KType;

typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_Byte;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_Short;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_Int;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_Long;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_Float;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_Double;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_Char;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_Boolean;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_Unit;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_UByte;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_UShort;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_UInt;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_ULong;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_Requests;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_Throwable;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_BuildConfig;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_FileUrl;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_collections_Map;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_Any;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_FileUrl_$serializer;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlinx_serialization_descriptors_SerialDescriptor;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_Array;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlinx_serialization_encoding_Decoder;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlinx_serialization_encoding_Encoder;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_FileUrl_Companion;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlinx_serialization_KSerializer;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_JsonAsString;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_Mapper;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlinx_serialization_json_Json;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_NiceResponse;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_com_fleeksoft_ksoup_nodes_Document;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_connections_Context;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_connections_anilist_Anilist;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_connections_anilist_AnilistQueries;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_connections_anilist_Genre;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlin_collections_List;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_AnimeParser;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_BaseParser;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_kotlinx_serialization_internal_SerializationConstructorMarker;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_BaseParser_$serializer;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_BaseParser_Companion;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_Episode;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_MangaChapter;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_MangaImage;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_MangaParser;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_NovelParser;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_R;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_R_string;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_ShowResponse;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_currContext;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_anime_extractors_JsUnpacker;
typedef struct {
  libsaikou_core_KNativePtr pinned;
} libsaikou_core_kref_ani_saikou_parsers_novel_Jsoup;

extern void saikouCoreInit();
extern void* searchAnime(void* query);

typedef struct {
  /* Service functions. */
  void (*DisposeStablePointer)(libsaikou_core_KNativePtr ptr);
  void (*DisposeString)(const char* string);
  libsaikou_core_KBoolean (*IsInstance)(libsaikou_core_KNativePtr ref, const libsaikou_core_KType* type);
  libsaikou_core_kref_kotlin_Byte (*createNullableByte)(libsaikou_core_KByte);
  libsaikou_core_KByte (*getNonNullValueOfByte)(libsaikou_core_kref_kotlin_Byte);
  libsaikou_core_kref_kotlin_Short (*createNullableShort)(libsaikou_core_KShort);
  libsaikou_core_KShort (*getNonNullValueOfShort)(libsaikou_core_kref_kotlin_Short);
  libsaikou_core_kref_kotlin_Int (*createNullableInt)(libsaikou_core_KInt);
  libsaikou_core_KInt (*getNonNullValueOfInt)(libsaikou_core_kref_kotlin_Int);
  libsaikou_core_kref_kotlin_Long (*createNullableLong)(libsaikou_core_KLong);
  libsaikou_core_KLong (*getNonNullValueOfLong)(libsaikou_core_kref_kotlin_Long);
  libsaikou_core_kref_kotlin_Float (*createNullableFloat)(libsaikou_core_KFloat);
  libsaikou_core_KFloat (*getNonNullValueOfFloat)(libsaikou_core_kref_kotlin_Float);
  libsaikou_core_kref_kotlin_Double (*createNullableDouble)(libsaikou_core_KDouble);
  libsaikou_core_KDouble (*getNonNullValueOfDouble)(libsaikou_core_kref_kotlin_Double);
  libsaikou_core_kref_kotlin_Char (*createNullableChar)(libsaikou_core_KChar);
  libsaikou_core_KChar (*getNonNullValueOfChar)(libsaikou_core_kref_kotlin_Char);
  libsaikou_core_kref_kotlin_Boolean (*createNullableBoolean)(libsaikou_core_KBoolean);
  libsaikou_core_KBoolean (*getNonNullValueOfBoolean)(libsaikou_core_kref_kotlin_Boolean);
  libsaikou_core_kref_kotlin_Unit (*createNullableUnit)(void);
  libsaikou_core_kref_kotlin_UByte (*createNullableUByte)(libsaikou_core_KUByte);
  libsaikou_core_KUByte (*getNonNullValueOfUByte)(libsaikou_core_kref_kotlin_UByte);
  libsaikou_core_kref_kotlin_UShort (*createNullableUShort)(libsaikou_core_KUShort);
  libsaikou_core_KUShort (*getNonNullValueOfUShort)(libsaikou_core_kref_kotlin_UShort);
  libsaikou_core_kref_kotlin_UInt (*createNullableUInt)(libsaikou_core_KUInt);
  libsaikou_core_KUInt (*getNonNullValueOfUInt)(libsaikou_core_kref_kotlin_UInt);
  libsaikou_core_kref_kotlin_ULong (*createNullableULong)(libsaikou_core_KULong);
  libsaikou_core_KULong (*getNonNullValueOfULong)(libsaikou_core_kref_kotlin_ULong);

  /* User functions. */
  struct {
    struct {
      struct {
        struct {
          struct {
            libsaikou_core_KType* (*_type)(void);
            libsaikou_core_kref_ani_saikou_BuildConfig (*_instance)();
            const char* (*get_MY_CUSTOM_API_KEY)(libsaikou_core_kref_ani_saikou_BuildConfig thiz);
          } BuildConfig;
          struct {
            struct {
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_FileUrl_$serializer (*_instance)();
              libsaikou_core_kref_kotlinx_serialization_descriptors_SerialDescriptor (*get_descriptor)(libsaikou_core_kref_ani_saikou_FileUrl_$serializer thiz);
              libsaikou_core_kref_kotlin_Array (*childSerializers)(libsaikou_core_kref_ani_saikou_FileUrl_$serializer thiz);
              libsaikou_core_kref_ani_saikou_FileUrl (*deserialize)(libsaikou_core_kref_ani_saikou_FileUrl_$serializer thiz, libsaikou_core_kref_kotlinx_serialization_encoding_Decoder decoder);
              void (*serialize)(libsaikou_core_kref_ani_saikou_FileUrl_$serializer thiz, libsaikou_core_kref_kotlinx_serialization_encoding_Encoder encoder, libsaikou_core_kref_ani_saikou_FileUrl value);
            } $serializer;
            struct {
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_FileUrl_Companion (*_instance)();
              libsaikou_core_kref_ani_saikou_FileUrl (*get)(libsaikou_core_kref_ani_saikou_FileUrl_Companion thiz, const char* url, libsaikou_core_kref_kotlin_collections_Map headers);
              libsaikou_core_kref_kotlinx_serialization_KSerializer (*serializer)(libsaikou_core_kref_ani_saikou_FileUrl_Companion thiz);
            } Companion;
            libsaikou_core_KType* (*_type)(void);
            libsaikou_core_kref_ani_saikou_FileUrl (*FileUrl)(const char* url, libsaikou_core_kref_kotlin_collections_Map headers);
            libsaikou_core_kref_kotlin_collections_Map (*get_headers)(libsaikou_core_kref_ani_saikou_FileUrl thiz);
            const char* (*get_url)(libsaikou_core_kref_ani_saikou_FileUrl thiz);
            const char* (*component1)(libsaikou_core_kref_ani_saikou_FileUrl thiz);
            libsaikou_core_kref_kotlin_collections_Map (*component2)(libsaikou_core_kref_ani_saikou_FileUrl thiz);
            libsaikou_core_kref_ani_saikou_FileUrl (*copy)(libsaikou_core_kref_ani_saikou_FileUrl thiz, const char* url, libsaikou_core_kref_kotlin_collections_Map headers);
            libsaikou_core_KBoolean (*equals)(libsaikou_core_kref_ani_saikou_FileUrl thiz, libsaikou_core_kref_kotlin_Any other);
            libsaikou_core_KInt (*hashCode)(libsaikou_core_kref_ani_saikou_FileUrl thiz);
            const char* (*toString)(libsaikou_core_kref_ani_saikou_FileUrl thiz);
          } FileUrl;
          struct {
            libsaikou_core_KType* (*_type)(void);
            libsaikou_core_kref_ani_saikou_JsonAsString (*JsonAsString)(const char* text);
            const char* (*get_text)(libsaikou_core_kref_ani_saikou_JsonAsString thiz);
          } JsonAsString;
          struct {
            libsaikou_core_KType* (*_type)(void);
            libsaikou_core_kref_ani_saikou_Mapper (*_instance)();
            libsaikou_core_kref_kotlinx_serialization_json_Json (*get_json)(libsaikou_core_kref_ani_saikou_Mapper thiz);
            const char* (*writeValueAsString)(libsaikou_core_kref_ani_saikou_Mapper thiz, libsaikou_core_kref_kotlin_Any obj);
          } Mapper;
          struct {
            libsaikou_core_KType* (*_type)(void);
            libsaikou_core_kref_ani_saikou_NiceResponse (*NiceResponse)(const char* url, const char* text);
            libsaikou_core_kref_com_fleeksoft_ksoup_nodes_Document (*get_document)(libsaikou_core_kref_ani_saikou_NiceResponse thiz);
            const char* (*get_text)(libsaikou_core_kref_ani_saikou_NiceResponse thiz);
            const char* (*get_url)(libsaikou_core_kref_ani_saikou_NiceResponse thiz);
          } NiceResponse;
          struct {
            libsaikou_core_KType* (*_type)(void);
            libsaikou_core_kref_ani_saikou_Requests (*Requests)();
            libsaikou_core_kref_ani_saikou_NiceResponse (*delete_)(libsaikou_core_kref_ani_saikou_Requests thiz, const char* url, libsaikou_core_kref_kotlin_collections_Map headers, libsaikou_core_kref_kotlin_collections_Map data, const char* referer, libsaikou_core_kref_ani_saikou_JsonAsString json, libsaikou_core_KLong timeout);
            libsaikou_core_kref_ani_saikou_NiceResponse (*get)(libsaikou_core_kref_ani_saikou_Requests thiz, const char* url, libsaikou_core_kref_kotlin_collections_Map headers, libsaikou_core_KLong timeout, const char* referer);
            libsaikou_core_kref_ani_saikou_NiceResponse (*post)(libsaikou_core_kref_ani_saikou_Requests thiz, const char* url, libsaikou_core_kref_kotlin_collections_Map headers, libsaikou_core_kref_kotlin_collections_Map data, const char* referer, libsaikou_core_kref_ani_saikou_JsonAsString json, libsaikou_core_KLong timeout);
            libsaikou_core_kref_ani_saikou_NiceResponse (*put)(libsaikou_core_kref_ani_saikou_Requests thiz, const char* url, libsaikou_core_kref_kotlin_collections_Map headers, libsaikou_core_kref_kotlin_collections_Map data, const char* referer, libsaikou_core_kref_ani_saikou_JsonAsString json, libsaikou_core_KLong timeout);
          } Requests;
          struct {
            struct {
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_connections_Context (*Context)();
              const char* (*getString)(libsaikou_core_kref_ani_saikou_connections_Context thiz, libsaikou_core_KInt id);
            } Context;
            struct {
              struct {
                libsaikou_core_KType* (*_type)(void);
                libsaikou_core_kref_ani_saikou_connections_anilist_Anilist (*_instance)();
              } Anilist;
              struct {
                libsaikou_core_KType* (*_type)(void);
                libsaikou_core_kref_ani_saikou_connections_anilist_AnilistQueries (*_instance)();
              } AnilistQueries;
              struct {
                libsaikou_core_KType* (*_type)(void);
                libsaikou_core_kref_ani_saikou_connections_anilist_Genre (*Genre)(const char* name, libsaikou_core_KInt id, const char* thumbnail, libsaikou_core_KLong time);
                libsaikou_core_KInt (*get_id)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz);
                void (*set_id)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz, libsaikou_core_KInt set);
                const char* (*get_name)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz);
                const char* (*get_thumbnail)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz);
                void (*set_thumbnail)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz, const char* set);
                libsaikou_core_KLong (*get_time)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz);
                void (*set_time)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz, libsaikou_core_KLong set);
                const char* (*component1)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz);
                libsaikou_core_KInt (*component2)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz);
                const char* (*component3)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz);
                libsaikou_core_KLong (*component4)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz);
                libsaikou_core_kref_ani_saikou_connections_anilist_Genre (*copy)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz, const char* name, libsaikou_core_KInt id, const char* thumbnail, libsaikou_core_KLong time);
                libsaikou_core_KBoolean (*equals)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz, libsaikou_core_kref_kotlin_Any other);
                libsaikou_core_KInt (*hashCode)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz);
                const char* (*toString)(libsaikou_core_kref_ani_saikou_connections_anilist_Genre thiz);
              } Genre;
            } anilist;
            void (*saveData)(const char* key, libsaikou_core_kref_kotlin_Any value);
          } connections;
          struct {
            struct {
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_parsers_AnimeParser (*AnimeParser)();
            } AnimeParser;
            struct {
              struct {
                libsaikou_core_KType* (*_type)(void);
                libsaikou_core_kref_ani_saikou_parsers_BaseParser_$serializer (*_instance)();
                libsaikou_core_kref_kotlinx_serialization_descriptors_SerialDescriptor (*get_descriptor)(libsaikou_core_kref_ani_saikou_parsers_BaseParser_$serializer thiz);
                libsaikou_core_kref_kotlin_Array (*childSerializers)(libsaikou_core_kref_ani_saikou_parsers_BaseParser_$serializer thiz);
                libsaikou_core_kref_ani_saikou_parsers_BaseParser (*deserialize)(libsaikou_core_kref_ani_saikou_parsers_BaseParser_$serializer thiz, libsaikou_core_kref_kotlinx_serialization_encoding_Decoder decoder);
                void (*serialize)(libsaikou_core_kref_ani_saikou_parsers_BaseParser_$serializer thiz, libsaikou_core_kref_kotlinx_serialization_encoding_Encoder encoder, libsaikou_core_kref_ani_saikou_parsers_BaseParser value);
              } $serializer;
              struct {
                libsaikou_core_KType* (*_type)(void);
                libsaikou_core_kref_ani_saikou_parsers_BaseParser_Companion (*_instance)();
                libsaikou_core_kref_kotlinx_serialization_KSerializer (*serializer)(libsaikou_core_kref_ani_saikou_parsers_BaseParser_Companion thiz);
              } Companion;
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_parsers_BaseParser (*BaseParser)(libsaikou_core_KInt seen1, const char* name, const char* saveName, const char* hostUrl, libsaikou_core_KBoolean isNSFW, libsaikou_core_kref_kotlinx_serialization_internal_SerializationConstructorMarker serializationConstructorMarker);
              libsaikou_core_kref_ani_saikou_parsers_BaseParser (*BaseParser_)();
              const char* (*get_hostUrl)(libsaikou_core_kref_ani_saikou_parsers_BaseParser thiz);
              libsaikou_core_KBoolean (*get_isNSFW)(libsaikou_core_kref_ani_saikou_parsers_BaseParser thiz);
              const char* (*get_name)(libsaikou_core_kref_ani_saikou_parsers_BaseParser thiz);
              const char* (*get_saveName)(libsaikou_core_kref_ani_saikou_parsers_BaseParser thiz);
            } BaseParser;
            struct {
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_parsers_Episode (*Episode)(const char* number, const char* link, const char* title, const char* thumbnail, const char* description, libsaikou_core_KBoolean isFiller, libsaikou_core_kref_kotlin_collections_Map extra);
              const char* (*get_description)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              libsaikou_core_kref_kotlin_collections_Map (*get_extra)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              libsaikou_core_KBoolean (*get_isFiller)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              const char* (*get_link)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              const char* (*get_number)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              const char* (*get_thumbnail)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              const char* (*get_title)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              const char* (*component1)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              const char* (*component2)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              const char* (*component3)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              const char* (*component4)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              const char* (*component5)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              libsaikou_core_KBoolean (*component6)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              libsaikou_core_kref_kotlin_collections_Map (*component7)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              libsaikou_core_kref_ani_saikou_parsers_Episode (*copy)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz, const char* number, const char* link, const char* title, const char* thumbnail, const char* description, libsaikou_core_KBoolean isFiller, libsaikou_core_kref_kotlin_collections_Map extra);
              libsaikou_core_KBoolean (*equals)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz, libsaikou_core_kref_kotlin_Any other);
              libsaikou_core_KInt (*hashCode)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
              const char* (*toString)(libsaikou_core_kref_ani_saikou_parsers_Episode thiz);
            } Episode;
            struct {
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_parsers_MangaChapter (*MangaChapter)(const char* name, const char* link);
              const char* (*get_link)(libsaikou_core_kref_ani_saikou_parsers_MangaChapter thiz);
              const char* (*get_name)(libsaikou_core_kref_ani_saikou_parsers_MangaChapter thiz);
              const char* (*component1)(libsaikou_core_kref_ani_saikou_parsers_MangaChapter thiz);
              const char* (*component2)(libsaikou_core_kref_ani_saikou_parsers_MangaChapter thiz);
              libsaikou_core_kref_ani_saikou_parsers_MangaChapter (*copy)(libsaikou_core_kref_ani_saikou_parsers_MangaChapter thiz, const char* name, const char* link);
              libsaikou_core_KBoolean (*equals)(libsaikou_core_kref_ani_saikou_parsers_MangaChapter thiz, libsaikou_core_kref_kotlin_Any other);
              libsaikou_core_KInt (*hashCode)(libsaikou_core_kref_ani_saikou_parsers_MangaChapter thiz);
              const char* (*toString)(libsaikou_core_kref_ani_saikou_parsers_MangaChapter thiz);
            } MangaChapter;
            struct {
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_parsers_MangaImage (*MangaImage)(const char* url);
              const char* (*get_url)(libsaikou_core_kref_ani_saikou_parsers_MangaImage thiz);
              const char* (*component1)(libsaikou_core_kref_ani_saikou_parsers_MangaImage thiz);
              libsaikou_core_kref_ani_saikou_parsers_MangaImage (*copy)(libsaikou_core_kref_ani_saikou_parsers_MangaImage thiz, const char* url);
              libsaikou_core_KBoolean (*equals)(libsaikou_core_kref_ani_saikou_parsers_MangaImage thiz, libsaikou_core_kref_kotlin_Any other);
              libsaikou_core_KInt (*hashCode)(libsaikou_core_kref_ani_saikou_parsers_MangaImage thiz);
              const char* (*toString)(libsaikou_core_kref_ani_saikou_parsers_MangaImage thiz);
            } MangaImage;
            struct {
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_parsers_MangaParser (*MangaParser)();
              const char* (*get_hostUrl)(libsaikou_core_kref_ani_saikou_parsers_MangaParser thiz);
              const char* (*get_name)(libsaikou_core_kref_ani_saikou_parsers_MangaParser thiz);
              const char* (*get_saveName)(libsaikou_core_kref_ani_saikou_parsers_MangaParser thiz);
            } MangaParser;
            struct {
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_parsers_NovelParser (*NovelParser)();
              const char* (*get_hostUrl)(libsaikou_core_kref_ani_saikou_parsers_NovelParser thiz);
              const char* (*get_name)(libsaikou_core_kref_ani_saikou_parsers_NovelParser thiz);
              const char* (*get_saveName)(libsaikou_core_kref_ani_saikou_parsers_NovelParser thiz);
            } NovelParser;
            struct {
              struct {
                libsaikou_core_KType* (*_type)(void);
                libsaikou_core_kref_ani_saikou_parsers_R_string (*_instance)();
                libsaikou_core_KInt (*get_backup_error)(libsaikou_core_kref_ani_saikou_parsers_R_string thiz);
              } string;
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_parsers_R (*_instance)();
            } R;
            struct {
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_parsers_ShowResponse (*ShowResponse)(const char* name, const char* link, const char* coverUrl);
              const char* (*get_coverUrl)(libsaikou_core_kref_ani_saikou_parsers_ShowResponse thiz);
              const char* (*get_link)(libsaikou_core_kref_ani_saikou_parsers_ShowResponse thiz);
              const char* (*get_name)(libsaikou_core_kref_ani_saikou_parsers_ShowResponse thiz);
              const char* (*component1)(libsaikou_core_kref_ani_saikou_parsers_ShowResponse thiz);
              const char* (*component2)(libsaikou_core_kref_ani_saikou_parsers_ShowResponse thiz);
              const char* (*component3)(libsaikou_core_kref_ani_saikou_parsers_ShowResponse thiz);
              libsaikou_core_kref_ani_saikou_parsers_ShowResponse (*copy)(libsaikou_core_kref_ani_saikou_parsers_ShowResponse thiz, const char* name, const char* link, const char* coverUrl);
              libsaikou_core_KBoolean (*equals)(libsaikou_core_kref_ani_saikou_parsers_ShowResponse thiz, libsaikou_core_kref_kotlin_Any other);
              libsaikou_core_KInt (*hashCode)(libsaikou_core_kref_ani_saikou_parsers_ShowResponse thiz);
              const char* (*toString)(libsaikou_core_kref_ani_saikou_parsers_ShowResponse thiz);
            } ShowResponse;
            struct {
              libsaikou_core_KType* (*_type)(void);
              libsaikou_core_kref_ani_saikou_parsers_currContext (*_instance)();
              libsaikou_core_kref_kotlin_Any (*invoke)(libsaikou_core_kref_ani_saikou_parsers_currContext thiz);
            } currContext;
            struct {
              struct {
                struct {
                  libsaikou_core_KType* (*_type)(void);
                  libsaikou_core_kref_ani_saikou_parsers_anime_extractors_JsUnpacker (*JsUnpacker)(const char* script);
                  const char* (*get_script)(libsaikou_core_kref_ani_saikou_parsers_anime_extractors_JsUnpacker thiz);
                  const char* (*unpack)(libsaikou_core_kref_ani_saikou_parsers_anime_extractors_JsUnpacker thiz);
                } JsUnpacker;
              } extractors;
            } anime;
            struct {
              struct {
                libsaikou_core_KType* (*_type)(void);
                libsaikou_core_kref_ani_saikou_parsers_novel_Jsoup (*_instance)();
                libsaikou_core_kref_com_fleeksoft_ksoup_nodes_Document (*parse)(libsaikou_core_kref_ani_saikou_parsers_novel_Jsoup thiz, const char* html);
              } Jsoup;
            } novel;
            libsaikou_core_kref_kotlin_collections_List (*getAnimeParsers)();
            libsaikou_core_kref_kotlin_collections_List (*getMangaParsers)();
            libsaikou_core_kref_kotlin_collections_List (*getNovelParsers)();
            void (*saveData)(const char* key, libsaikou_core_kref_kotlin_Any value);
            void (*snackString)(const char* msg, const char* act, const char* msg2);
            void (*toast)(const char* msg);
            void (*updateSources)();
          } parsers;
          libsaikou_core_kref_ani_saikou_Requests (*get_client)();
          void (*set_client)(libsaikou_core_kref_ani_saikou_Requests set);
          void (*initializeNetwork)();
          void (*logError)(libsaikou_core_kref_kotlin_Throwable e, libsaikou_core_KBoolean post, libsaikou_core_KBoolean snackbar);
          void (*saikouCoreInit_)();
          void* (*searchAnime_)(void* query);
          const char* (*findBetween)(const char* thiz, const char* a, const char* b);
        } saikou;
      } ani;
    } root;
  } kotlin;
} libsaikou_core_ExportedSymbols;
extern libsaikou_core_ExportedSymbols* libsaikou_core_symbols(void);
#ifdef __cplusplus
}  /* extern "C" */
#endif
#endif  /* KONAN_LIBSAIKOU_CORE_H */
