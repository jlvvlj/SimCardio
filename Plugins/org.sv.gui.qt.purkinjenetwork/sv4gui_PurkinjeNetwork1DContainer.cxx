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

#include "sv4gui_PurkinjeNetwork1DContainer.h"
#include "math.h"

#include <berryIPreferencesService.h>
#include <berryIPreferences.h>
#include <berryPlatform.h>

sv4guiPurkinjeNetwork1DContainer::sv4guiPurkinjeNetwork1DContainer()
{
  MITK_INFO << "[sv4guiPurkinjeNetwork1DContainer::sv4guiPurkinjeNetwork1DContainer] ";
  m_startSeeds = std::vector<std::vector<double>>();
  m_endSeeds = std::vector<std::vector<std::vector<double>>>();
  hoverPoint.push_back(0.0);
  hoverPoint.push_back(0.0);
  hoverPoint.push_back(0.0);
  m_NewSurfaceNetworkMesh = true;
  m_SurfaceNetworkMesh = nullptr;
}

sv4guiPurkinjeNetwork1DContainer::sv4guiPurkinjeNetwork1DContainer(const sv4guiPurkinjeNetwork1DContainer& other)
  :BaseData(other)
{
  int numStartSeeds = other.getNumStartSeeds();
  for(int start = 0; start < numStartSeeds; start++){

    std::vector<double> startSeed  = other.getStartSeed(start);
    addStartSeed(startSeed[0], startSeed[1], startSeed[2]);

    int numEndSeeds = other.getNumEndSeeds(start);
    for (int end = 0; end < numEndSeeds; end++){
      std::vector<double> endSeed = other.getEndSeed(start, end);
      addEndSeed(start, endSeed[0], endSeed[1], endSeed[2]);
    }
  }
};

sv4guiPurkinjeNetwork1DContainer::~sv4guiPurkinjeNetwork1DContainer(){

};

//-------------------------------
// Get/Set NewSurfaceNetworkMesh
//-------------------------------

bool sv4guiPurkinjeNetwork1DContainer::IsNewSurfaceNetworkMesh()
{
  return m_NewSurfaceNetworkMesh;
}
void sv4guiPurkinjeNetwork1DContainer::SetNewSurfaceNetworkMesh(bool value)
{
  m_NewSurfaceNetworkMesh = value;
}


sv4guiMesh* sv4guiPurkinjeNetwork1DContainer::GetSurfaceNetworkMesh()
{
  return m_SurfaceNetworkMesh;
}

void sv4guiPurkinjeNetwork1DContainer::SetSurfaceNetworkMesh(sv4guiMesh* mesh)
{
  m_SurfaceNetworkMesh = mesh;
  m_NewSurfaceNetworkMesh = true;
}

void sv4guiPurkinjeNetwork1DContainer::addStartSeed(double x, double y, double z)
{
  auto v = std::vector<double>();
  v.push_back(x);
  v.push_back(y);
  v.push_back(z);
  m_startSeeds.push_back(v);

  auto v2 = std::vector<std::vector<double>>();
  m_endSeeds.push_back(v2);
};

void sv4guiPurkinjeNetwork1DContainer::addEndSeed(double x, double y, double z, int seedIndex)
{
  auto v = std::vector<double>();
  v.push_back(x);
  v.push_back(y);
  v.push_back(z);

  m_endSeeds[seedIndex].push_back(v);

};

int sv4guiPurkinjeNetwork1DContainer::getNumStartSeeds() const {
  return m_startSeeds.size();
}

int sv4guiPurkinjeNetwork1DContainer::getNumEndSeeds(int startSeedIndex) const {
  return m_endSeeds[startSeedIndex].size();
}

std::vector<double> sv4guiPurkinjeNetwork1DContainer::getStartSeed(int seedIndex) const {
  return m_startSeeds[seedIndex];
}

std::vector<double> sv4guiPurkinjeNetwork1DContainer::getEndSeed(int startSeedIndex, int endSeedIndex) const{
  return m_endSeeds[startSeedIndex][endSeedIndex];
}

std::vector<int> sv4guiPurkinjeNetwork1DContainer::findNearestSeed(double x, double y, double z, double tol){

  bool done = false;

  int numStartSeeds = getNumStartSeeds();

  auto v = std::vector<int>();
  v.push_back(-1);
  v.push_back(-1);

  if (numStartSeeds == 0) return v;

  for (int start = 0; start < numStartSeeds; start++){
    auto v_start = m_startSeeds[start];
    auto d       = distance(v_start[0], v_start[1], v_start[2], x, y ,z);

    if (d < tol) {
      v[0] = start;
      v[1] = -1;
      return v;
    }

    int numEndSeeds = getNumEndSeeds(start);
    for (int end = 0; end < numEndSeeds; end++){
      auto v_end = m_endSeeds[start][end];

      auto d     = distance(v_end[0], v_end[1], v_end[2], x, y, z);
      if (d < tol){
        v[0] = start;
        v[1] = end;
        return v;
      }
    }
  }
  return v;
}

void sv4guiPurkinjeNetwork1DContainer::deleteSeed(int startIndex, int endIndex){
  std::cout << "Deleting " << startIndex << " " << endIndex << "\n";
  std::cout << "startSeeds size " << m_startSeeds.size() << "\n";

  if (m_startSeeds.size() <= startIndex)
    return;

  if (endIndex == -1){
    m_startSeeds.erase(m_startSeeds.begin()+startIndex);
    if (!(m_endSeeds.size() <= startIndex))
      m_endSeeds.erase(m_endSeeds.begin()+startIndex);

    return;
  }

  std::vector<std::vector<double>>& seedVec = m_endSeeds[startIndex];
  seedVec.erase(seedVec.begin()+endIndex);
  return;
}

double sv4guiPurkinjeNetwork1DContainer::distance(double x1, double y1, double z1, double x2,
  double y2, double z2) const{

    auto d1 = (x1-x2)*(x1-x2);
    auto d2 = (y1-y2)*(y1-y2);
    auto d3 = (z1-z2)*(z1-z2);
    return sqrt(d1+d2+d3);
}
