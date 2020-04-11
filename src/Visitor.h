#ifndef K_VISITOR_H_
#define K_VISITOR_H_

#include <type_traits>
#include "template_magic.h"

template<typename... Types>
class Visitor : public Visitor<Types>... {
public:
  using Visitor<Types>::visit...;
};

template<typename T>
class Visitor<T> {
public:
  virtual void visit(T& visitable) = 0;
};

template<typename Derived, typename Base, typename Visitor>
class Visitable : public Base {
public:
  void accept(Visitor& visitor) override {
    visitor.visit(static_cast<Derived&>(*this));
  }
};

namespace detail {
  template<typename Visitor, typename... Types>
  class DefaultVisitor;

  template<typename Visitor, typename Head>
  class DefaultVisitor<Visitor, Head> : public Visitor {
  public:
    void visit(Head& visitable) override {}
  };

  template<typename Visitor, typename Head, typename... Tail>
  class DefaultVisitor<Visitor, Head, Tail...> : public DefaultVisitor<Visitor, Tail...> {
  public:
    using DefaultVisitor<Visitor, Tail...>::visit;
    void visit(Head& visitable) override {
      using parent = find_parent_v<Head, Tail...>;
      if constexpr (!std::is_void_v<parent>) {
        visit(static_cast<parent&>(visitable));
      }
    }
  };

  template<typename>
  struct default_visitor_helper;

  template<typename... Types>
  struct default_visitor_helper<Visitor<Types...>> {
    using type = DefaultVisitor<Visitor<Types...>, Types...>;
  };
}

template<typename... Types>
using DefaultVisitor = typename detail::default_visitor_helper<Types...>::type;

#endif
