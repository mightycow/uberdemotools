#if defined(_MSC_VER)
#	pragma warning(push, 0)
#endif
#if defined(__GNUC__) || defined(__clang__)
#	pragma GCC diagnostic push
#endif
#if defined(__GNUC__) || defined(__clang__)
#	pragma GCC diagnostic ignored "-Wmisleading-indentation"
#	pragma GCC diagnostic ignored "-Wshift-negative-value"
#	pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#	pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#	pragma GCC diagnostic ignored "-Wunused-parameter"
#	pragma GCC diagnostic ignored "-Wformat"
#endif
#if defined(__GNUC__) && !defined(__clang__)
#	pragma GCC diagnostic ignored "-Wstringop-overflow"
#	pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
