#ifndef K_TEMPLATE_MAGIC_H_
#define K_TEMPLATE_MAGIC_H_

// Select type T if flag == true else U
template<bool flag, typename T, typename U>
struct select_type {
  using type = T;
};

template<typename T, typename U>
struct select_type<false, T, U> {
  using type = U;
};

// Syntactic sugar for select
template<bool flag, typename T, typename U>
using select_type_v = typename select_type<flag, T, U>::type;

// Finds first base of For in list In
template<typename For, typename... In>
struct find_parent {
  using type = void;
};

// Syntactic sugar for find_parent
template<typename For, typename... In>
using find_parent_v = typename find_parent<For, In...>::type;

template<typename For, typename Head, typename... Tail>
struct find_parent<For, Head, Tail...> {
  using type = select_type_v<std::is_base_of_v<Head, For>,
                             Head,
                             find_parent_v<For, Tail...>>;
};

#endif
