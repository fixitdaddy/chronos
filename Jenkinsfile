pipeline {
  agent any

  options {
    timestamps()
    ansiColor('xterm')
    buildDiscarder(logRotator(numToKeepStr: '20'))
    timeout(time: 45, unit: 'MINUTES')
  }

  environment {
    DEBIAN_FRONTEND = 'noninteractive'
  }

  stages {
    stage('Checkout') {
      steps {
        checkout scm
      }
    }

    stage('Build and Test (containerized)') {
      steps {
        sh '''#!/usr/bin/env bash
          set -euxo pipefail

          docker run --rm \
            -v "$PWD:/src" \
            -w /src \
            ubuntu:24.04 \
            bash -lc '
              set -euxo pipefail
              apt-get update
              apt-get install -y --no-install-recommends build-essential cmake ninja-build g++

              cmake -S . -B build -G Ninja -DCHRONOS_BUILD_TESTS=ON
              cmake --build build --parallel
              ctest --test-dir build --output-on-failure
            '
        '''
      }
    }

    stage('Syntax Guard') {
      steps {
        sh '''#!/usr/bin/env bash
          set -euxo pipefail

          docker run --rm \
            -v "$PWD:/src" \
            -w /src \
            ubuntu:24.04 \
            bash -lc '
              set -euxo pipefail
              apt-get update
              apt-get install -y --no-install-recommends g++ findutils

              for f in $(find shared-core api-server scheduler worker tests -type f -name "*.cpp" | sort); do
                echo "Checking $f"
                g++ -std=c++20 -fsyntax-only -Ishared-core/include -Iapi-server/include -Ischeduler/include -Iworker/include "$f"
              done
            '
        '''
      }
    }

    stage('Compose Validate') {
      steps {
        sh '''#!/usr/bin/env bash
          set -euxo pipefail
          docker compose -f docker-compose.yml config >/tmp/chronos-compose.validated.yml
        '''
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: 'docs/**,ops/**,scripts/**,.github/**,Jenkinsfile', allowEmptyArchive: true
    }
    success {
      echo 'jenkins ci passed ✅'
    }
    failure {
      echo 'jenkins ci failed ❌'
    }
  }
}
