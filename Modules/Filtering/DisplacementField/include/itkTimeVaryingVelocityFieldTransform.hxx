/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef __itkTimeVaryingVelocityFieldTransform_hxx
#define __itkTimeVaryingVelocityFieldTransform_hxx

#include "itkTimeVaryingVelocityFieldTransform.h"

#include "itkTimeVaryingVelocityFieldIntegrationImageFilter.h"
#include "itkVectorLinearInterpolateImageFunction.h"

namespace itk
{

/**
 * Constructor
 */
template<class TScalar, unsigned int NDimensions>
TimeVaryingVelocityFieldTransform<TScalar, NDimensions>
::TimeVaryingVelocityFieldTransform()
{
  this->m_LowerTimeBound = 0.0;
  this->m_UpperTimeBound = 1.0;
  this->m_NumberOfIntegrationSteps = 10;
  this->m_IntegrateTimeVaryingVelocityField = true;
  this->m_TimeVaryingVelocityFieldSetTime = 0;

  this->m_TimeVaryingVelocityField = NULL;

  // Setup and assign parameter helper. This will hold the time varying velocity
  // field for access through the common TransformParameters interface.
  TransformParametersHelperType * helper = new TransformParametersHelperType;

  // After assigning this, parameters will manage this deleting when appropriate.
  this->m_Parameters.SetHelper( helper );

  typedef VectorLinearInterpolateImageFunction
    <TimeVaryingVelocityFieldType, ScalarType> DefaultInterpolatorType;

  this->m_TimeVaryingVelocityFieldInterpolator = DefaultInterpolatorType::New();
}

/**
 * Destructor
 */
template<class TScalar, unsigned int NDimensions>
TimeVaryingVelocityFieldTransform<TScalar, NDimensions>::
~TimeVaryingVelocityFieldTransform()
{
}

/** Leave CreateAnother undefined. To fully implement here, it must be
 * sure to copy all members. It may be called from transform-cloning
 * that only copies parameters, so override here to prevent
 * its use without copying full members. */
template<class TScalar, unsigned int NDimensions>
::itk::LightObject::Pointer
TimeVaryingVelocityFieldTransform<TScalar, NDimensions>
::CreateAnother() const
{
  itkExceptionMacro( "CreateAnother unimplemented. See source comments." );
}

/**
 * return an inverse transformation
 */
template<class TScalar, unsigned int NDimensions>
bool
TimeVaryingVelocityFieldTransform<TScalar, NDimensions>
::GetInverse( Self *inverse ) const
{
  if ( !inverse || !this->m_TimeVaryingVelocityField )
    {
    return false;
    }
  else
    {
    inverse->SetIntegrateTimeVaryingVelocityField( false );
    inverse->SetTimeVaryingVelocityField( this->m_TimeVaryingVelocityField );
    inverse->SetUpperTimeBound( this->m_LowerTimeBound );
    inverse->SetLowerTimeBound( this->m_UpperTimeBound );
    inverse->SetTimeVaryingVelocityFieldInterpolator( this->m_TimeVaryingVelocityFieldInterpolator );
    inverse->SetDisplacementField( this->m_InverseDisplacementField );
    inverse->SetInverseDisplacementField( this->m_DisplacementField );
    inverse->SetInterpolator( this->m_Interpolator );
    inverse->SetIntegrateTimeVaryingVelocityField( true );
    return true;
    }
}

// Return an inverse of this transform
template<class TScalar, unsigned int NDimensions>
typename TimeVaryingVelocityFieldTransform<TScalar, NDimensions>::
  InverseTransformBasePointer
TimeVaryingVelocityFieldTransform<TScalar, NDimensions>
::GetInverseTransform() const
{
  Pointer inverseTransform = New();
  if( this->GetInverse( inverseTransform ) )
    {
    return inverseTransform.GetPointer();
    }
  else
    {
    return NULL;
    }
}

template<class TScalar, unsigned int NDimensions>
void
TimeVaryingVelocityFieldTransform<TScalar, NDimensions>
::IntegrateVelocityField()
{
  if( this->m_IntegrateTimeVaryingVelocityField )
    {
    typedef TimeVaryingVelocityFieldIntegrationImageFilter
      <TimeVaryingVelocityFieldType, DisplacementFieldType> IntegratorType;

    typename IntegratorType::Pointer integrator = IntegratorType::New();
    integrator->SetInput( this->m_TimeVaryingVelocityField );
    integrator->SetLowerTimeBound( this->m_LowerTimeBound );
    integrator->SetUpperTimeBound( this->m_UpperTimeBound );

    if( !this->m_TimeVaryingVelocityFieldInterpolator.IsNull() )
      {
      integrator->SetVelocityFieldInterpolator( this->m_TimeVaryingVelocityFieldInterpolator );
      }

    integrator->SetNumberOfIntegrationSteps( this->m_NumberOfIntegrationSteps );
    integrator->Update();

    DisplacementFieldType * deformationField = integrator->GetOutput();

    deformationField->DisconnectPipeline();

    this->SetDisplacementField( deformationField );
    this->GetInterpolator()->SetInputImage( deformationField );

    typename IntegratorType::Pointer inverseIntegrator = IntegratorType::New();
    inverseIntegrator->SetInput( this->m_TimeVaryingVelocityField );
    inverseIntegrator->SetLowerTimeBound( this->m_UpperTimeBound );
    inverseIntegrator->SetUpperTimeBound( this->m_LowerTimeBound );
    if( !this->m_TimeVaryingVelocityFieldInterpolator.IsNull() )
      {
      integrator->SetVelocityFieldInterpolator( this->m_TimeVaryingVelocityFieldInterpolator );
      }

    inverseIntegrator->SetNumberOfIntegrationSteps( this->m_NumberOfIntegrationSteps );
    inverseIntegrator->Update();

    DisplacementFieldType * inverseDisplacementField = inverseIntegrator->GetOutput();
    inverseDisplacementField->DisconnectPipeline();

    this->SetInverseDisplacementField( inverseDisplacementField );
    }
}

template<class TScalar, unsigned int NDimensions>
void
TimeVaryingVelocityFieldTransform<TScalar, NDimensions>
::SetParameters(const ParametersType & params)
{
  if( &(this->m_Parameters) != &params )
    {
    if( params.Size() != this->m_Parameters.Size() )
      {
      itkExceptionMacro( "Input parameters size (" << params.Size()
        << ") does not match internal size ("
        << this->m_Parameters.Size() << ")." );
      }
    /* copy into existing object */
    this->m_Parameters = params;
    this->Modified();
    }
}


template<class TScalar, unsigned int NDimensions>
void
TimeVaryingVelocityFieldTransform<TScalar, NDimensions>
::UpdateTransformParameters( DerivativeType & update, ScalarType factor )
{
  //This simply adds the values.
  //TODO: This should be multi-threaded probably, via image add filter.
  Superclass::UpdateTransformParameters( update, factor );
}

template<class TScalar, unsigned int NDimensions>
void TimeVaryingVelocityFieldTransform<TScalar, NDimensions>
::SetTimeVaryingVelocityField( TimeVaryingVelocityFieldType * field )
{
  itkDebugMacro( "Setting TimeVaryingVelocityField to " << field );
  if ( this->m_TimeVaryingVelocityField != field )
    {
    this->m_TimeVaryingVelocityField = field;
    this->Modified();
    this->m_TimeVaryingVelocityFieldSetTime = this->GetMTime();
    if( !this->m_TimeVaryingVelocityFieldInterpolator.IsNull() )
      {
      this->m_TimeVaryingVelocityFieldInterpolator->SetInputImage(
        this->m_TimeVaryingVelocityField );
      }
    // Assign to parameters object
    this->m_Parameters.SetParametersObject( this->m_TimeVaryingVelocityField );


    }
}

template <class TScalar, unsigned int NDimensions>
void
TimeVaryingVelocityFieldTransform<TScalar, NDimensions>
::PrintSelf( std::ostream& os, Indent indent ) const
{
  Superclass::PrintSelf( os,indent );

  std::cout << indent << "TimeVaryingVelocityFieldInterpolator: " << std::endl;
  std::cout << indent << indent << this->m_TimeVaryingVelocityFieldInterpolator
    << std::endl;

  std::cout << indent << "TimeVaryingVelocityField: " << std::endl;
  std::cout << indent << indent << this->m_TimeVaryingVelocityField
    << std::endl;

  os << indent << "LowerTimeBound: " << this->m_LowerTimeBound << std::endl;
  os << indent << "UpperTimeBound: " << this->m_UpperTimeBound << std::endl;
  os << indent << "NumberOfIntegrationSteps: "
    << this->m_NumberOfIntegrationSteps << std::endl;
}
} // namespace itk

#endif
