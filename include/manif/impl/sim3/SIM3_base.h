#ifndef _MANIF_MANIF_SIM3_BASE_H_
#define _MANIF_MANIF_SIM3_BASE_H_

#include "manif/impl/SIM3/SIM3_properties.h"
#include "manif/impl/manifold_base.h"
#include "manif/impl/so3/SO3_map.h"

namespace manif
{

////////////////
///          ///
/// Manifold ///
///          ///
////////////////

template <typename _Derived>
struct SIM3Base : ManifoldBase<_Derived>
{
private:

  using Base = ManifoldBase<_Derived>;
  using Type = SIM3Base<_Derived>;

public:

  MANIF_MANIFOLD_PROPERTIES

  MANIF_MANIFOLD_TYPEDEF

  /// @todo find a mechanism to fetch it from base
  /// just like the other typedefs
  using Translation = typename internal::traits<_Derived>::Translation;

  /// Manifold common API

  Transformation transform() const;
  Rotation rotation() const;
  Translation translation() const;

  SIM3Base<_Derived>& setIdentity();

  Manifold inverse(OptJacobianRef J_minv_m = {}) const;
  Tangent lift(OptJacobianRef J_t_m = {}) const;

  template <typename _DerivedOther>
  Manifold compose(const ManifoldBase<_DerivedOther>& m,
                   OptJacobianRef J_mc_ma = {},
                   OptJacobianRef J_mc_mb = {}) const;

  Vector act(const Vector &v,
             OptJacobianRef J_vout_m = {},
             OptJacobianRef J_vout_v = {}) const;

  Jacobian adj() const;

  using Base::coeffs;
  using Base::coeffs_nonconst;
  using Base::data;
  MANIF_INHERIT_MANIFOLD_AUTO_API
  MANIF_INHERIT_MANIFOLD_OPERATOR

  /// SIM3 specific functions

  Scalar x() const;
  Scalar y() const;
  Scalar z() const;

  //Scalar roll() const;
  //Scalar pitch() const;
  //Scalar yaw() const;

//protected:

  /// Helper

//  auto trapart()
//  -> decltype( std::declval<Type>().coeffs().template block<Dim, 1>(0,0) )
//  {
//    coeffs().block<Dim, 1>(0,0);
//  }

//  auto trapart() const
//  -> decltype( std::declval<const Type>().coeffs().template block<Dim, 1>(0,0) )
//  {
//    coeffs().block<Dim, 1>(0,0);
//  }

//  auto rotpart() const
//  -> decltype( std::declval<const Type>().coeffs().template block<4, 1>(3,0) )
//  {
//    coeffs().block<4, 1>(3,0);
//  }

//  auto rotpart() const
//  -> decltype( std::declval<const Type>().coeffs().template block<4, 1>(3,0) )
//  {
//    coeffs().block<4, 1>(3,0);
//  }

  Eigen::Map<const SO3<Scalar>> asSO3() const
  {
    return Eigen::Map<const SO3<Scalar>>(coeffs().data()+3);
  }

  Eigen::Map<SO3<Scalar>> asSO3()
  {
    return Eigen::Map<SO3<Scalar>>(coeffs_nonconst().data()+3);
  }
};

template <typename _Derived>
typename SIM3Base<_Derived>::Transformation
SIM3Base<_Derived>::transform() const
{
  Transformation T = Transformation::Identity();
  T.template topLeftCorner<3,3>()  = rotation();
  T.template topRightCorner<3,1>() = translation();
  return T;
}

template <typename _Derived>
typename SIM3Base<_Derived>::Rotation
SIM3Base<_Derived>::rotation() const
{
  return asSO3().rotation();
}

template <typename _Derived>
typename SIM3Base<_Derived>::Translation
SIM3Base<_Derived>::translation() const
{
  return coeffs().template head<3>();
}

template <typename _Derived>
SIM3Base<_Derived>&
SIM3Base<_Derived>::setIdentity()
{
  coeffs_nonconst().setZero();
  asSO3().setIdentity();
  return *this;
}

template <typename _Derived>
typename SIM3Base<_Derived>::Manifold
SIM3Base<_Derived>::inverse(OptJacobianRef J_minv_m) const
{
  if (J_minv_m)
  {
    (*J_minv_m) = -adj();
  }

  const SO3<Scalar> so3inv = asSO3().inverse();

  return Manifold(-so3inv.act(translation()),
                   so3inv);
}

template <typename _Derived>
typename SIM3Base<_Derived>::Tangent
SIM3Base<_Derived>::lift(OptJacobianRef J_t_m) const
{
  using std::abs;
  using std::sqrt;

  const SO3Tangent<Scalar> so3tan = asSO3().lift();

  Tangent tan((typename Tangent::DataType() <<
               so3tan.ljac().inverse()*translation(),
               so3tan.coeffs()).finished());

  if (J_t_m)
  {
    // Jr^-1
    (*J_t_m) = tan.rjac().inverse();
  }

  return tan;
}

template <typename _Derived>
template <typename _DerivedOther>
typename SIM3Base<_Derived>::Manifold
SIM3Base<_Derived>::compose(
    const ManifoldBase<_DerivedOther>& m,
    OptJacobianRef J_mc_ma,
    OptJacobianRef J_mc_mb) const
{
  static_assert(
    std::is_base_of<SIM3Base<_DerivedOther>, _DerivedOther>::value,
    "Argument does not inherit from SIM3Base !");

  const auto& m_SIM3 = static_cast<const SIM3Base<_DerivedOther>&>(m);

  if (J_mc_ma)
  {
    (*J_mc_ma) = m.adj().inverse();
  }

  if (J_mc_mb)
  {
    J_mc_mb->setIdentity();
  }

  return Manifold(rotation()*m_SIM3.translation() + translation(),
                  asSO3().compose(m_SIM3.asSO3()).quat());
}

template <typename _Derived>
typename SIM3Base<_Derived>::Vector
SIM3Base<_Derived>::act(const Vector &v,
                       OptJacobianRef J_vout_m,
                       OptJacobianRef J_vout_v) const
{
  if (J_vout_m)
  {
    MANIF_NOT_IMPLEMENTED_YET
    (*J_vout_v) = -skew(v);
  }

  if (J_vout_v)
  {
    (*J_vout_v) = rotation();
  }

  return transform() * v;
}

template <typename _Derived>
typename SIM3Base<_Derived>::Jacobian
SIM3Base<_Derived>::adj() const
{
  /// @note Chirikjian (close to Eq.10.94)
  /// says
  ///       Ad(g) = |  R  0 |
  ///               | T.R R |
  ///
  /// considering vee(log(g)) = (w;v)
  /// with T = [t]_x
  ///
  /// but this is
  ///       Ad(g) = | R T.R |
  ///               | 0  R  |
  ///
  /// considering vee(log(g)) = (v;w)

  Jacobian Adj = Jacobian::Zero();
  Adj.template topLeftCorner<3,3>() = rotation();
  Adj.template bottomRightCorner<3,3>() =
      Adj.template topLeftCorner<3,3>();
  Adj.template topRightCorner<3,3>() =
    skew(translation()) * Adj.template topLeftCorner<3,3>();

  return Adj;
}

/// SIM3 specific function

template <typename _Derived>
typename SIM3Base<_Derived>::Scalar
SIM3Base<_Derived>::x() const
{
  return coeffs().x();
}

template <typename _Derived>
typename SIM3Base<_Derived>::Scalar
SIM3Base<_Derived>::y() const
{
  return coeffs().y();
}

template <typename _Derived>
typename SIM3Base<_Derived>::Scalar
SIM3Base<_Derived>::z() const
{
  return coeffs().z();
}

} /* namespace manif */

#endif /* _MANIF_MANIF_SIM3_BASE_H_ */
