/* Copyright (c) Stanford University, The Regents of the University of
 *               California, and others.
 *
 * All Rights Reserved.
 *
 * See Copyright-SimVascular.txt for additional details.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sv4gui_PurkinjeNetworkUtils.h"


#include <itkVTKImageToImageFilter.h>
#include <itkThresholdImageFilter.h>
#include <itkCastImageFilter.h>

#include <itkImageToVTKImageFilter.h>
#include <itkVTKImageToImageFilter.h>

#include <itkMetaImageIO.h>
#include <itkImageFileWriter.h>
#include "itkGradientMagnitudeRecursiveGaussianImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkGeodesicActiveContourLevelSetImageFilter.h"
#include "itkRecursiveGaussianImageFilter.h"
#include "itkGradientAnisotropicDiffusionImageFilter.h"
#include "itkBinaryFillholeImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include <itkConnectedThresholdImageFilter.h>
#include "itkImageDuplicator.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkResampleImageFilter.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkIdentityTransform.h"
#include "itkMultiplyImageFilter.h"
#include "itkAddImageFilter.h"
#include "itkGrayscaleDilateImageFilter.h"
#include "itkGrayscaleErodeImageFilter.h"
#include "itkBinaryBallStructuringElement.h"
#include <itkThresholdImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkCollidingFrontsImageFilter.h>
#include <itkMinimumImageFilter.h>

#include <vtkMarchingCubes.h>
#include <vtkImageCast.h>
#include "vtkPolyDataConnectivityFilter.h"
#include "vtkCellLocator.h"
#include <vtkMetaImageWriter.h>

sv4guiPurkinjeNetworkUtils::sv4guiPurkinjeNetworkUtils(){

}

sv4guiPurkinjeNetworkUtils::~sv4guiPurkinjeNetworkUtils(){

}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::vtkImageToItkImage(vtkImageData* imageData){
  auto caster = vtkSmartPointer<vtkImageCast>::New();
  caster->SetInputData(imageData);
  caster->SetOutputScalarTypeToFloat();
  caster->Update();

  auto VTK2ITK = itk::VTKImageToImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType>::New();
  VTK2ITK->SetInput(caster->GetOutput());
  VTK2ITK->Update();

  auto itkImage = VTK2ITK->GetOutput();
  return itkImage;
}

vtkSmartPointer<vtkImageData> sv4guiPurkinjeNetworkUtils::itkImageToVtkImage(sv4guiPurkinjeNetworkUtils::itkImPoint image){
  auto ITK2VTK = itk::ImageToVTKImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType>::New();
  ITK2VTK->SetInput(image);
  ITK2VTK->Update();

  auto vtkImage = ITK2VTK->GetOutput();
  return vtkImage;
}

vtkSmartPointer<vtkPolyData> sv4guiPurkinjeNetworkUtils::marchingCubes(vtkImageData* imageData, double isovalue, bool largest_cc){
  auto MC = vtkSmartPointer<vtkMarchingCubes>::New();
  MC->SetInputData(imageData);
  MC->SetValue(0,isovalue);
  MC->Update();

  if (!largest_cc){
    auto pd = MC->GetOutput();
    return pd;
  }else{
    vtkSmartPointer<vtkPolyDataConnectivityFilter> connectivityFilter =
      vtkSmartPointer<vtkPolyDataConnectivityFilter>::New();
    connectivityFilter->SetInputConnection(MC->GetOutputPort());
    connectivityFilter->SetExtractionModeToLargestRegion();
    connectivityFilter->Update();
    return connectivityFilter->GetOutput();
  }

}

vtkSmartPointer<vtkPolyData> sv4guiPurkinjeNetworkUtils::seedMarchingCubes(vtkImageData* imageData, double isovalue,
double px, double py, double pz){
  auto MC = vtkSmartPointer<vtkMarchingCubes>::New();
  MC->SetInputData(imageData);
  MC->SetValue(0,isovalue);
  MC->Update();

  auto cellLocator = vtkSmartPointer<vtkCellLocator>::New();
  cellLocator->SetDataSet(MC->GetOutput());
  cellLocator->BuildLocator();

  double testPoint[3] = {px, py, pz};
  double closestPointDistance;
  double closestPoint[3];
  vtkIdType cellId;
  int subId;

  cellLocator->FindClosestPoint(testPoint, closestPoint, cellId, subId, closestPointDistance);

  std::cout << "Vtk connectivity filter\n";
  vtkSmartPointer<vtkPolyDataConnectivityFilter> polyDataConnectivityFilter
  = vtkSmartPointer<vtkPolyDataConnectivityFilter>::New();
  polyDataConnectivityFilter->SetExtractionModeToCellSeededRegions();
  polyDataConnectivityFilter->SetInputData(MC->GetOutput());
  polyDataConnectivityFilter->InitializeSeedList();
  polyDataConnectivityFilter->AddSeed(cellId);
  std::cout << "Running connectivity filter\n";
  polyDataConnectivityFilter->Update();

  std::cout << "get connectivity output\n";
  auto pd = polyDataConnectivityFilter->GetOutput();
  return pd;
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::copyImage(sv4guiPurkinjeNetworkUtils::itkImPoint image){
  auto dup = itk::ImageDuplicator<sv4guiPurkinjeNetworkUtils::itkImageType>::New();

  dup->SetInputImage(image);
  dup->Update();
  return dup->GetOutput();
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::elementwiseMinimum(sv4guiPurkinjeNetworkUtils::itkImPoint image1,
  sv4guiPurkinjeNetworkUtils::itkImPoint image2){

    auto min = itk::MinimumImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType,
      sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType>::New();

    min->SetInput(0, image1);
    min->SetInput(1, image2);
    min->Update();
    return min->GetOutput();
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::threshold(sv4guiPurkinjeNetworkUtils::itkImPoint image,
  double lowerThreshold, double upperThreshold){

  auto thresh = itk::ThresholdImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType>::New();
  thresh->SetInput(image);
  thresh->ThresholdOutside(lowerThreshold, upperThreshold);
  thresh->SetOutsideValue(0.0);
  thresh->Update();
  auto itkImage = thresh->GetOutput();
  return itkImage;
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::binaryThreshold(sv4guiPurkinjeNetworkUtils::itkImPoint image,
  double lowerThreshold, double upperThreshold, double insideValue, double outsideValue){

  auto thresh = itk::BinaryThresholdImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType>::New();
  thresh->SetInput(image);
  thresh->SetLowerThreshold(lowerThreshold);
  thresh->SetUpperThreshold(upperThreshold);
  thresh->SetOutsideValue(outsideValue);
  thresh->SetInsideValue(insideValue);
  thresh->Update();
  auto itkImage = thresh->GetOutput();
  return itkImage;
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::zeroLevel(sv4guiPurkinjeNetworkUtils::itkImPoint image,
  double pixelValue){

  auto multiply = itk::MultiplyImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType>::New();

  multiply->SetInput(image);
  multiply->SetConstant(-1.0);
  multiply->Update();

  auto add = itk::AddImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType>::New();

  add->SetInput(multiply->GetOutput());
  add->SetConstant2(pixelValue);
  add->Update();

  auto itkImage = add->GetOutput();
  return itkImage;
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::connectedThreshold(sv4guiPurkinjeNetworkUtils::itkImPoint image,
  double lowerThreshold, double upperThreshold, double insideValue,
  std::vector<std::vector<int>> seeds){

  auto thresh = itk::ConnectedThresholdImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType>::New();
  thresh->SetInput(image);
  thresh->SetLower(lowerThreshold);
  thresh->SetUpper(upperThreshold);
  thresh->SetReplaceValue(insideValue);

  for (int i = 0; i < seeds.size(); i++){
    auto v = seeds[i];
    sv4guiPurkinjeNetworkUtils::itkImageType::IndexType index = {{v[0],v[1],v[2]}};
    thresh->AddSeed(index);
  }


  thresh->Update();
  auto itkImage = thresh->GetOutput();
  return itkImage;
}

std::vector<int> sv4guiPurkinjeNetworkUtils::physicalPointToIndex(sv4guiPurkinjeNetworkUtils::itkImPoint image,
  double x, double y, double z){

  sv4guiPurkinjeNetworkUtils::itkImageType::PointType p;
  p[0] = x;
  p[1] = y;
  p[2] = z;

  sv4guiPurkinjeNetworkUtils::itkImageType::IndexType index;

  image->TransformPhysicalPointToIndex(p,index);

  auto v = std::vector<int>();
  v.push_back(index[0]);
  v.push_back(index[1]);
  v.push_back(index[2]);
  return v;
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::collidingFronts(sv4guiPurkinjeNetworkUtils::itkImPoint image,
  int x1, int y1, int z1, int x2, int y2, int z2,
    double lowerThreshold, double upperThreshold){

  typedef sv4guiPurkinjeNetworkUtils::itkImageType CFImageType;

  auto thresh = itk::ThresholdImageFilter<CFImageType>::New();

  thresh->SetInput(image);
  thresh->ThresholdOutside(lowerThreshold,upperThreshold);
  thresh->SetOutsideValue(0.0);
  thresh->Update();

  //finally CF needs pixel values to be between 0 and 1, so we rescale the image
  auto scaler = itk::RescaleIntensityImageFilter<CFImageType,CFImageType>::New();
  scaler->SetInput(thresh->GetOutput());
  scaler->SetOutputMinimum(0.0);
  scaler->SetOutputMaximum(1.0);
  scaler->Update();

  //now construct collidingfronts filter
  typedef itk::CollidingFrontsImageFilter<CFImageType,CFImageType> CFType;
  auto CF = CFType::New();

  //Colliding fronts expects seeds as level set nodes in a node container,
  //so we convert the input seed vectors to this type
  typedef CFType::NodeContainer NodeContainer;
  typedef CFType::NodeType NodeType;

  //create node container for starting seed points
  auto seedContainer1 = NodeContainer::New();
  seedContainer1->Initialize();

  CFImageType::IndexType index;

  index[0] = x1;
  index[1] = y1;
  index[2] = z1;

  NodeType n;
  n.SetIndex(index);
  n.SetValue(0.0);
  seedContainer1->InsertElement(0,n);

  //create node container for end seed points
  auto seedContainer2 = NodeContainer::New();
  seedContainer2->Initialize();

  CFImageType::IndexType index2;

  index2[0] = x2;
  index2[1] = y2;
  index2[2] = z2;

  NodeType n2;
  n2.SetIndex(index2);
  n2.SetValue(0.0);
  seedContainer2->InsertElement(0,n2);

  //now set the inputs for the colliding fronts filter
  CF->SetInput(scaler->GetOutput());
  CF->SetSeedPoints1(seedContainer1);
  CF->SetSeedPoints2(seedContainer2);
  // CF->ApplyConnectivityOn();
  CF->StopOnTargetsOn();
  CF->Update();

  thresh->SetInput(CF->GetOutput());
  thresh->ThresholdAbove(-1e-12);
  thresh->SetOutsideValue(1.0);
  thresh->Update();

  auto itkImage = thresh->GetOutput();
  return itkImage;
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::openClose(sv4guiPurkinjeNetworkUtils::itkImPoint image,
   int radius){

  sv4guiPurkinjeNetworkUtils::StructElType structuringElement;
  structuringElement.SetRadius(radius);
  structuringElement.CreateStructuringElement();

  auto erode = itk::GrayscaleErodeImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::StructElType>::New();
  erode->SetInput(image);
  erode->SetKernel(structuringElement);
  erode->Update();

  auto dilate = itk::GrayscaleDilateImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::StructElType>::New();
  dilate->SetInput(erode->GetOutput());
  dilate->SetKernel(structuringElement);
  dilate->Update();

  auto itkImage = dilate->GetOutput();
  return itkImage;
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::editImage(sv4guiPurkinjeNetworkUtils::itkImPoint image,
  int ox, int oy, int oz, int l, int w, int h, double replaceValue){

  auto duplicator = itk::ImageDuplicator<sv4guiPurkinjeNetworkUtils::itkImageType>::New();
  duplicator->SetInputImage(image);
  duplicator->Update();
  auto clonedImage = duplicator->GetOutput();

  int x1 = ox - l/2;
  int y1 = oy - w/2;
  int z1 = oz - h/2;

  int x2 = ox + l/2;
  int y2 = oy + w/2;
  int z2 = oz + h/2;

  for (int i = x1; i < x2; i++){
    for (int j = y1; j < y2; j++){
      for (int k = z1; k < z2; k++){
        sv4guiPurkinjeNetworkUtils::itkImageType::IndexType Index;
        Index[0] = i;
        Index[1] = j;
        Index[2] = k;

        clonedImage->SetPixel(Index, replaceValue);
      }
    }
  }

  return clonedImage;
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::cropImage(sv4guiPurkinjeNetworkUtils::itkImPoint image,
  int ox, int oy, int oz, int l, int w, int h){

  auto crop = itk::RegionOfInterestImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType, itkImageType>::New();

  int x1 = ox - l/2;
  int y1 = oy - w/2;
  int z1 = oz - h/2;

  sv4guiPurkinjeNetworkUtils::itkImageType::IndexType start;
  start[0] = x1;
  start[1] = y1;
  start[2] = z1;

  sv4guiPurkinjeNetworkUtils::itkImageType::SizeType size;
  size[0] = l;
  size[1] = w;
  size[2] = h;

  sv4guiPurkinjeNetworkUtils::itkImageType::RegionType region;
  region.SetIndex(start);
  region.SetSize(size);

  crop->SetInput(image);
  crop->SetRegionOfInterest(region);
  crop->Update();
  return crop->GetOutput();
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::resampleImage(sv4guiPurkinjeNetworkUtils::itkImPoint
  image, double space_x, double space_y, double space_z){

  auto resample = itk::ResampleImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType>::New();

  auto interpolator = itk::BSplineInterpolateImageFunction<sv4guiPurkinjeNetworkUtils::itkImageType, double, double>::New();

  auto identity = itk::IdentityTransform<double, 3>::New();
  identity->SetIdentity();


  //get required image information for resample
  auto origin = image->GetOrigin();

  double outputSpacing[3] = {space_x, space_y, space_z};

  auto inputSpacing = image->GetSpacing();

  auto inputRegion = image->GetLargestPossibleRegion();
  auto inputSize = inputRegion.GetSize();

  itk::Size<3> outputSize;
  outputSize[0] = (int)(inputSize[0]*inputSpacing[0]/outputSpacing[0]);
  outputSize[1] = (int)(inputSize[1]*inputSpacing[1]/outputSpacing[1]);
  outputSize[2] = (int)(inputSize[2]*inputSpacing[2]/outputSpacing[2]);

  //set resample stuff
  resample->SetTransform(identity);
  resample->SetInterpolator(interpolator);
  resample->SetOutputOrigin(origin);
  resample->SetSize(outputSize);
  resample->SetOutputSpacing(outputSpacing);
  resample->SetInput(image);

  resample->Update();
  return resample->GetOutput();
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::fillHoles(sv4guiPurkinjeNetworkUtils::itkImPoint image,
  double foregroundValue){

  auto fill = itk::BinaryFillholeImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType>::New();
  fill->SetInput(image);
  fill->SetForegroundValue(foregroundValue);
  fill->Update();
  auto itkImage = fill->GetOutput();
  return itkImage;
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::gradientMagnitude(sv4guiPurkinjeNetworkUtils::itkImPoint image, double sigma){
  auto gradientFilter = itk::GradientMagnitudeRecursiveGaussianImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType>::New();

  gradientFilter->SetSigma(sigma);
  gradientFilter->SetInput(image);
  gradientFilter->Update();

  auto rescaleFilter = itk::RescaleIntensityImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType,sv4guiPurkinjeNetworkUtils::itkImageType>::New();
  rescaleFilter->SetInput(gradientFilter->GetOutput());
  rescaleFilter->SetOutputMinimum(0.0);
  rescaleFilter->SetOutputMaximum(1.0);
  rescaleFilter->Update();
  auto itkImage = rescaleFilter->GetOutput();
  return itkImage;
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::smooth(sv4guiPurkinjeNetworkUtils::itkImPoint image, double sigma){
  auto smooth = itk::RecursiveGaussianImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType>::New();

  smooth->SetSigma(sigma);
  smooth->SetInput(image);
  smooth->Update();

  auto itkImage = smooth->GetOutput();
  return itkImage;
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::anisotropicSmooth(sv4guiPurkinjeNetworkUtils::itkImPoint image,
  int iterations, double timeStep, double conductance){
  auto smooth = itk::GradientAnisotropicDiffusionImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType,
    sv4guiPurkinjeNetworkUtils::itkImageType>::New();

  smooth->SetNumberOfIterations(iterations);
  smooth->SetTimeStep(timeStep);
  smooth->SetConductanceParameter(conductance);
  smooth->SetInput(image);
  smooth->Update();

  auto itkImage = smooth->GetOutput();
  return itkImage;
}

sv4guiPurkinjeNetworkUtils::itkImPoint sv4guiPurkinjeNetworkUtils::geodesicLevelSet(sv4guiPurkinjeNetworkUtils::itkImPoint initialization, sv4guiPurkinjeNetworkUtils::itkImPoint edgeImage, double propagation, double advection, double curvature, int iterations){
  auto levelSetFilter = itk::GeodesicActiveContourLevelSetImageFilter<sv4guiPurkinjeNetworkUtils::itkImageType, sv4guiPurkinjeNetworkUtils::itkImageType>::New();

  levelSetFilter->SetPropagationScaling(propagation);
  levelSetFilter->SetAdvectionScaling(advection);
  levelSetFilter->SetCurvatureScaling(curvature);
  levelSetFilter->SetNumberOfIterations(iterations);
  levelSetFilter->SetInitialImage(initialization);
  levelSetFilter->SetFeatureImage(edgeImage);
  levelSetFilter->SetMaximumRMSError(1e-20);

  levelSetFilter->Update();
  auto itkImage = levelSetFilter->GetOutput();
  return itkImage;
}

void sv4guiPurkinjeNetworkUtils::writeMHA(sv4guiPurkinjeNetworkUtils::itkImPoint image, std::string filename){
  auto writer = itk::ImageFileWriter<sv4guiPurkinjeNetworkUtils::itkImageType>::New();
  itk::MetaImageIO::Pointer metaWriter = itk::MetaImageIO::New();
  writer->SetImageIO( metaWriter );

  metaWriter->SetDataFileName( "LOCAL" );
  writer->SetFileName( filename );
  writer->SetInput( image );

  writer->Write();
}

void sv4guiPurkinjeNetworkUtils::writeVtkMHA(vtkImageData* vtkImage, std::string filename){
  auto writer = vtkMetaImageWriter::New();
  writer->SetInputData(vtkImage);
  writer->SetFileName(filename.c_str());
  writer->Update();
  writer->Write();
}
